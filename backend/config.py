"""
Configuration settings for the Image Compression Lab backend
"""
from pathlib import Path
from pydantic_settings import BaseSettings


class Settings(BaseSettings):
    """Application settings using environment variables"""
    
    # App settings
    app_name: str = "Interactive Image Compression Lab"
    app_version: str = "1.0.0"
    debug: bool = False
    
    # Server settings
    host: str = "0.0.0.0"
    port: int = 8000
    
    # File upload settings
    max_upload_size: int = 500 * 1024 * 1024  # 500 MB
    allowed_extensions: list = ["jpg", "jpeg", "png", "bmp"]
    
    # Storage settings
    upload_dir: Path = Path(__file__).parent / "storage" / "uploads"
    compressed_dir: Path = Path(__file__).parent / "storage" / "compressed"
    
    # CORS settings
    cors_origins: list = ["http://localhost:3000", "http://localhost:3001", "http://localhost:8080"]
    
    # Database settings (PostgreSQL)
    database_url: str = "postgresql://user:password@localhost:5432/compression_lab"
    
    class Config:
        env_file = ".env"
        case_sensitive = False


settings = Settings()

# Ensure storage directories exist
settings.upload_dir.mkdir(parents=True, exist_ok=True)
settings.compressed_dir.mkdir(parents=True, exist_ok=True)
