"""
Compression API routes and endpoints

Integration with Huffman compression engine:
  - The Huffman engine is available in services.compression.demo
  - Use compress_data() and decompress_data() for compression operations
  - See HUFFMAN_IMPLEMENTATION.md for detailed integration examples
  
Example usage in background tasks:
  from services.compression.demo import compress_data, decompress_data
  
  # In your compression task:
  compressed, metadata, padding, stats = compress_data(pixel_bytes)
"""
from fastapi import APIRouter, UploadFile, File, HTTPException, BackgroundTasks
from fastapi.responses import FileResponse
import logging
import time
import io
import base64
from ..utils import (
    generate_job_id,
    save_uploaded_file,
    get_file_size,
    get_validation_errors,
)
from ..services import ImageProcessor
from ..models import UploadResponse, ImageInfo
from pathlib import Path
from PIL import Image
import numpy as np

# NOTE: Huffman compression imports to be used in background tasks
# from services.compression.demo import compress_data, decompress_data

logger = logging.getLogger(__name__)
router = APIRouter(prefix="/api/compression", tags=["compression"])

# In-memory job tracking (will be replaced with database)
compression_jobs = {}


def perform_compression(job_id: str, quality: str = 'high'):
    """Perform actual image compression"""
    try:
        job = compression_jobs[job_id]
        
        # Load original image
        original_path = job['filepath']
        original_image = Image.open(original_path)
        original_size = Path(original_path).stat().st_size
        
        # Get image info
        img_width, img_height = original_image.size
        img_format = original_image.format or 'PNG'
        img_mode = original_image.mode
        
        # Determine compression parameters
        quality_map = {
            'high': {'quality': 90, 'resize_ratio': 1.0},
            'medium': {'quality': 75, 'resize_ratio': 0.9},
            'fast': {'quality': 60, 'resize_ratio': 0.8},
        }
        params = quality_map.get(quality, quality_map['high'])
        
        # Create compressed version
        compressed_image = original_image.copy()
        
        # Resize if needed
        if params['resize_ratio'] < 1.0:
            new_width = int(original_image.width * params['resize_ratio'])
            new_height = int(original_image.height * params['resize_ratio'])
            compressed_image = compressed_image.resize((new_width, new_height), Image.Resampling.LANCZOS)
        
        # Save compressed image
        storage_dir = Path('storage/compressed')
        storage_dir.mkdir(parents=True, exist_ok=True)
        compressed_path = storage_dir / f"{job_id}_compressed.jpg"

        # JPEG does not support alpha or palette modes directly.
        # Convert to RGB to avoid "cannot write mode RGBA as JPEG" errors
        jpeg_image = compressed_image
        if jpeg_image.mode not in ("RGB", "L"):
            jpeg_image = jpeg_image.convert("RGB")

        jpeg_image.save(str(compressed_path), quality=params['quality'], optimize=True)
        compressed_size = compressed_path.stat().st_size
        
        # Convert images to base64 for display
        # Original
        original_buffer = io.BytesIO()
        original_image.save(original_buffer, format='PNG')
        original_base64 = base64.b64encode(original_buffer.getvalue()).decode('utf-8')
        
        # Compressed
        compressed_buffer = io.BytesIO()
        compressed_image.save(compressed_buffer, format='PNG')
        compressed_base64 = base64.b64encode(compressed_buffer.getvalue()).decode('utf-8')
        
        # Calculate metrics
        compression_time = time.time() - job.get('compression_start', time.time())

        # Bytes saved (never negative). If the recompressed file is larger,
        # we treat it as "no additional savings" for reporting purposes.
        saved_bytes = max(0, original_size - compressed_size)
        savings_percent = (saved_bytes / original_size * 100) if original_size > 0 else 0

        if compressed_size > 0:
            raw_ratio = original_size / compressed_size
        else:
            raw_ratio = 0

        compression_ratio = 1.0 if saved_bytes == 0 else raw_ratio
        
        # Helper function to format file sizes
        def format_file_size(bytes_size):
            for unit in ['B', 'KB', 'MB', 'GB']:
                if bytes_size < 1024:
                    return f"{bytes_size:.2f} {unit}"
                bytes_size /= 1024
            return f"{bytes_size:.2f} TB"
        
        # Build comprehensive metrics
        metrics = {
            'file_sizes': {
                'original_bytes': original_size,
                'compressed_bytes': compressed_size,
                'original_formatted': format_file_size(original_size),
                'compressed_formatted': format_file_size(compressed_size),
                'saved_formatted': format_file_size(saved_bytes),
            },
            'compression': {
                'ratio': round(compression_ratio, 2),
                'percentage': round(savings_percent, 2),
                'compression_time_ms': round(compression_time * 1000, 2),
                'decompression_time_ms': 0,  # Set on decompression
            },
            'image_info': {
                'original_width': img_width,
                'original_height': img_height,
                'format': img_format,
                'color_mode': img_mode,
                'compressed_width': compressed_image.width,
                'compressed_height': compressed_image.height,
            },
            'timestamp': {
                'start': job.get('upload_time', time.time()),
                'end': time.time(),
                'duration_ms': round(compression_time * 1000, 2),
            },
        }
        
        # Update job with results
        compression_jobs[job_id].update({
            'status': 'completed',
            'original_size': original_size,
            'compressed_size': compressed_size,
            'compression_time': compression_time,
            'compression_ratio': compression_ratio,
            'savings_percent': savings_percent,
            'original_base64': original_base64,
            'compressed_base64': compressed_base64,
            'compressed_path': str(compressed_path),
            'metrics': metrics,
        })
        
        logger.info(f"Compression completed: {job_id} ({savings_percent:.1f}% savings)")
        
    except Exception as e:
        logger.error(f"Compression failed for job {job_id}: {e}")
        if job_id in compression_jobs:
            compression_jobs[job_id]['status'] = 'failed'
            compression_jobs[job_id]['error'] = str(e)


@router.post("/upload", response_model=UploadResponse)
async def upload_image(file: UploadFile = File(...)):
    """
    Upload an image file for compression
    
    Args:
        file: Image file to upload (jpg, png, bmp)
        
    Returns:
        UploadResponse with job ID and image metadata
        
    Raises:
        HTTPException: If validation fails
    """
    try:
        # Read file content
        content = await file.read()
        file_size = len(content)
        
        # Load image to get metadata
        import io
        from PIL import Image
        
        image = Image.open(io.BytesIO(content))
        width, height = image.size
        
        # Validate file
        validation_errors = get_validation_errors(
            file.filename,
            file_size,
            width,
            height
        )
        
        if validation_errors:
            raise HTTPException(
                status_code=400,
                detail=f"Validation failed: {'; '.join(validation_errors)}"
            )
        
        # Generate job ID
        job_id = generate_job_id()
        
        # Save uploaded file
        filepath = save_uploaded_file(content, f"{job_id}_{file.filename}")
        
        # Get image info
        image_processor = ImageProcessor()
        image_info = image_processor.get_image_info(image, file.filename, file_size)
        
        # Track job
        compression_jobs[job_id] = {
            'filename': file.filename,
            'filepath': filepath,
            'status': 'uploaded',
            'file_size': file_size,
            'image_info': image_info,
            'upload_time': time.time(),
        }
        
        logger.info(f"Image uploaded successfully: {file.filename} (Job ID: {job_id})")
        
        return UploadResponse(
            job_id=job_id,
            filename=file.filename,
            file_size=file_size,
            image_info=image_info,
            message=f"Image uploaded successfully. Job ID: {job_id}"
        )
    
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"Error uploading file: {str(e)}")
        raise HTTPException(
            status_code=500,
            detail=f"Error uploading file: {str(e)}"
        )


@router.get("/job/{job_id}")
async def get_job_status(job_id: str):
    """
    Get compression job status
    
    Args:
        job_id: Job ID to query
        
    Returns:
        Job status and metadata
        
    Raises:
        HTTPException: If job not found
    """
    if job_id not in compression_jobs:
        raise HTTPException(
            status_code=404,
            detail=f"Job not found: {job_id}"
        )
    
    job = compression_jobs[job_id]
    response = {
        'job_id': job_id,
        'filename': job['filename'],
        'status': job['status'],
        'file_size': job['file_size'],
    }
    
    # Include results if compression is completed
    if job['status'] == 'completed':
        response['metrics'] = job.get('metrics', {})
        response['original_image'] = f"data:image/png;base64,{job.get('original_base64', '')}"
        response['compressed_image'] = f"data:image/png;base64,{job.get('compressed_base64', '')}"
    elif job['status'] == 'failed':
        response['error'] = job.get('error', 'Unknown error')
    
    return response


@router.post("/compress/{job_id}")
async def compress_image(
    job_id: str,
    background_tasks: BackgroundTasks,
    quality: str = "high",
    enable_preprocessing: bool = True
):
    """
    Start compression for an uploaded image
    
    Args:
        job_id: Job ID to compress
        quality: Compression quality ('high', 'medium', 'fast')
        background_tasks: FastAPI background tasks
        enable_preprocessing: Whether to apply preprocessing
        
    Returns:
        Compression job details
        
    Raises:
        HTTPException: If job not found or already processing
    """
    if job_id not in compression_jobs:
        raise HTTPException(
            status_code=404,
            detail=f"Job not found: {job_id}"
        )
    
    job = compression_jobs[job_id]
    
    if job['status'] != 'uploaded':
        raise HTTPException(
            status_code=400,
            detail=f"Job status is {job['status']}, cannot compress"
        )
    
    # Update status with compression settings
    compression_jobs[job_id]['status'] = 'processing'
    compression_jobs[job_id]['quality'] = quality
    compression_jobs[job_id]['compression_start'] = time.time()
    
    # Perform compression immediately (synchronously for now)
    perform_compression(job_id, quality)

    # If compression failed, surface this as an HTTP error so the
    # frontend can show a clear message instead of an empty preview.
    job_after = compression_jobs[job_id]
    if job_after.get('status') != 'completed':
        error_msg = job_after.get('error', 'Compression failed')
        raise HTTPException(status_code=500, detail=error_msg)

    return {
        'job_id': job_id,
        'message': 'Compression completed',
        'status': job_after['status'],
        'quality': quality,
        'metrics': job_after.get('metrics', {})
    }


@router.get("/compare/{job_id}")
async def get_comparison(job_id: str):
    """
    Get original and compressed images for comparison
    
    Args:
        job_id: Job ID to retrieve images for
        
    Returns:
        Paths to original and compressed images
        
    Raises:
        HTTPException: If job not found or not completed
    """
    if job_id not in compression_jobs:
        raise HTTPException(
            status_code=404,
            detail=f"Job not found: {job_id}"
        )
    
    job = compression_jobs[job_id]
    
    if job['status'] != 'completed':
        raise HTTPException(
            status_code=400,
            detail=f"Job status is {job['status']}, cannot retrieve comparison"
        )
    
    return {
        'job_id': job_id,
        'original_path': job['filepath'],
        'compressed_path': job.get('compressed_filepath'),
        'metrics': job.get('metrics'),
    }


@router.get("/metrics/{job_id}")
async def get_compression_metrics(job_id: str):
    """
    Get compression metrics for completed job
    
    Args:
        job_id: Job ID to retrieve metrics for
        
    Returns:
        Compression metrics and statistics
        
    Raises:
        HTTPException: If job not found or not completed
    """
    if job_id not in compression_jobs:
        raise HTTPException(
            status_code=404,
            detail=f"Job not found: {job_id}"
        )
    
    job = compression_jobs[job_id]
    
    if job['status'] != 'completed':
        raise HTTPException(
            status_code=400,
            detail=f"Job status is {job['status']}, metrics not available"
        )
    
    return job.get('metrics', {})


@router.get("/download/{job_id}")
async def download_compressed_image(job_id: str):
    """
    Download the compressed image
    
    Args:
        job_id: Job ID to download compressed image from
        
    Returns:
        FileResponse with compressed image
        
    Raises:
        HTTPException: If job not found or not completed
    """
    if job_id not in compression_jobs:
        raise HTTPException(
            status_code=404,
            detail=f"Job not found: {job_id}"
        )
    
    job = compression_jobs[job_id]
    
    if job['status'] != 'completed':
        raise HTTPException(
            status_code=400,
            detail=f"Job status is {job['status']}, cannot download"
        )
    
    compressed_path = job.get('compressed_path')
    if not compressed_path or not Path(compressed_path).exists():
        raise HTTPException(
            status_code=404,
            detail="Compressed file not found"
        )
    
    # Get original filename and create compressed version
    original_filename = job['filename']
    name, ext = original_filename.rsplit('.', 1) if '.' in original_filename else (original_filename, 'jpg')
    download_filename = f"{name}_compressed.{ext}"
    
    return FileResponse(
        path=compressed_path,
        filename=download_filename,
        media_type='image/jpeg'
    )


@router.delete("/job/{job_id}")
async def delete_job(job_id: str):
    """
    Delete a compression job and cleanup files
    
    Args:
        job_id: Job ID to delete
        
    Returns:
        Deletion confirmation
        
    Raises:
        HTTPException: If job not found
    """
    if job_id not in compression_jobs:
        raise HTTPException(
            status_code=404,
            detail=f"Job not found: {job_id}"
        )
    
    job = compression_jobs[job_id]
    
    # TODO: Cleanup files
    # from utils import delete_file
    # delete_file(Path(job['filepath']))
    # if 'compressed_filepath' in job:
    #     delete_file(Path(job['compressed_filepath']))
    
    del compression_jobs[job_id]
    
    return {
        'job_id': job_id,
        'message': 'Job deleted successfully'
    }
