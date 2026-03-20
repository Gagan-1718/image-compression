"""Initialize compression package"""
import time
from typing import Tuple
import numpy as np
from .huffman import (
    Node,
    HuffmanTree,
    CompressionStats,
    build_frequency_table,
    build_huffman_tree,
    generate_codes,
    encode_pixels,
    decode_pixels,
    serialize_tree,
    deserialize_tree,
)


class HuffmanCompressionService:
    """Service for Huffman encoding and decoding of image data"""
    
    def __init__(self):
        """Initialize compression service"""
        self.compression_tree = None
        self.frequency_table = None
    
    def compress(self, pixel_data: np.ndarray) -> Tuple[bytes, dict]:
        """
        Compress pixel data using Huffman encoding
        
        Args:
            pixel_data: Flattened pixel data array (numpy array)
            
        Returns:
            Tuple of (compressed_bytes, metadata)
        """
        start_time = time.time()
        
        # Convert numpy array to bytes
        if isinstance(pixel_data, np.ndarray):
            pixel_bytes = pixel_data.astype(np.uint8).tobytes()
            original_dtype = str(pixel_data.dtype)
            original_shape = pixel_data.shape
        else:
            pixel_bytes = pixel_data
            original_dtype = 'uint8'
            original_shape = (len(pixel_data),)
        
        # Build frequency table
        self.frequency_table = build_frequency_table(pixel_bytes)
        
        # Build Huffman tree
        self.compression_tree = build_huffman_tree(self.frequency_table)
        
        # Encode data
        compressed_bytes, padding = encode_pixels(pixel_bytes, self.compression_tree.codes)
        
        # Serialize tree metadata
        tree_metadata = serialize_tree(self.compression_tree)
        
        compression_time = (time.time() - start_time) * 1000  # Convert to ms
        
        metadata = {
            'original_size': len(pixel_bytes),
            'compressed_size': len(compressed_bytes),
            'compression_time_ms': compression_time,
            'tree_metadata': tree_metadata,
            'padding': padding,
            'original_dtype': original_dtype,
            'original_shape': original_shape,
            'unique_symbols': len(self.frequency_table),
        }
        
        return compressed_bytes, metadata
    
    def decompress(self, compressed_bytes: bytes, metadata: dict) -> Tuple[np.ndarray, float]:
        """
        Decompress Huffman encoded data
        
        Args:
            compressed_bytes: Compressed data bytes
            metadata: Compression metadata
            
        Returns:
            Tuple of (decompressed_array, decompression_time_ms)
        """
        start_time = time.time()
        
        # Deserialize tree
        tree_metadata = metadata.get('tree_metadata')
        compression_tree = deserialize_tree(tree_metadata)
        
        # Decode pixels
        original_size = metadata['original_size']
        padding = metadata['padding']
        decompressed_bytes = decode_pixels(compressed_bytes, compression_tree, padding)
        
        # Convert back to numpy array with original dtype and shape
        dtype = np.dtype(metadata.get('original_dtype', 'uint8'))
        shape = metadata.get('original_shape', (len(decompressed_bytes),))
        decompressed_array = np.frombuffer(decompressed_bytes[:original_size], dtype=dtype).reshape(shape)
        
        decompression_time = (time.time() - start_time) * 1000  # Convert to ms
        
        return decompressed_array, decompression_time


__all__ = [
    "Node",
    "HuffmanTree",
    "CompressionStats",
    "build_frequency_table",
    "build_huffman_tree",
    "generate_codes",
    "encode_pixels",
    "decode_pixels",
    "serialize_tree",
    "deserialize_tree",
    "HuffmanCompressionService",
]
