use crate::error::{CaesiumCliError, CliResult, ErrorCode};
use crate::options::{format_human_size, DEFAULT_MAX_INPUT_BYTES};
use image::ImageReader;
use img_parts::{Bytes, DynImage, ImageEXIF, ImageICC};
use schemars::JsonSchema;
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::fs;
use std::io::Cursor;
use std::path::Path;

#[derive(Debug, Clone, Serialize, Deserialize, JsonSchema)]
pub struct ImageInfo {
    pub path: String,
    pub file_name: String,
    pub format: String,
    pub mime: String,
    pub width: u32,
    pub height: u32,
    pub exif_orientation: u32,
    pub effective_width: u32,
    pub effective_height: u32,
    pub total_pixels: u64,
    pub megapixels: f64,
    pub size_bytes: u64,
    pub human_size: String,
    pub has_exif: bool,
    pub has_icc_profile: bool,
    pub sha256: String,
    pub supported: bool,
    pub warnings: Vec<String>,
}

/// Reads EXIF orientation tag (1..=8), returning 1 if absent or unreadable.
pub fn read_exif_orientation(data: &[u8]) -> u32 {
    let reader = exif::Reader::new();
    let mut cursor = Cursor::new(data);
    let Ok(exif_data) = reader.read_from_container(&mut cursor) else {
        return 1;
    };
    let Some(field) = exif_data.get_field(exif::Tag::Orientation, exif::In::PRIMARY) else {
        return 1;
    };
    field.value.get_uint(0).unwrap_or(1)
}

/// Returns true if EXIF orientation swaps width and height (orientations 5, 6, 7, 8).
pub fn is_rotated_by_exif(orientation: u32) -> bool {
    matches!(orientation, 5..=8)
}

/// Inspects an image on disk without mutating anything.
pub fn inspect_path(path: &Path) -> CliResult<ImageInfo> {
    if !path.exists() {
        return Err(CaesiumCliError::new(
            ErrorCode::InvalidInput,
            format!("File does not exist: {}", path.display()),
        )
        .with_path(path.to_string_lossy()));
    }

    let raw = fs::read(path)?;
    let canonical = path.canonicalize().unwrap_or_else(|_| path.to_path_buf());
    let file_name = canonical
        .file_name()
        .map(|s| s.to_string_lossy().to_string())
        .unwrap_or_default();
    inspect_bytes_with_meta(&raw, &canonical.to_string_lossy(), &file_name)
}

/// Inspects raw image bytes in memory (used for stdin or buffer probes).
pub fn inspect_bytes(data: &[u8], label: Option<&str>) -> CliResult<ImageInfo> {
    let name = label.unwrap_or("<stdin>");
    inspect_bytes_with_meta(data, name, name)
}

fn inspect_bytes_with_meta(data: &[u8], path_label: &str, file_name: &str) -> CliResult<ImageInfo> {
    let size_bytes = data.len() as u64;
    let mut warnings = Vec::new();
    if size_bytes > DEFAULT_MAX_INPUT_BYTES {
        warnings.push(format!(
            "File size ({}) exceeds GUI 500 MB default guard",
            format_human_size(size_bytes as i64)
        ));
    }

    let (format, mime, supported) = match infer::get(data) {
        Some(kind) => match kind.mime_type() {
            "image/jpeg" => ("jpg".to_string(), "image/jpeg".to_string(), true),
            "image/png" => ("png".to_string(), "image/png".to_string(), true),
            "image/webp" => ("webp".to_string(), "image/webp".to_string(), true),
            "image/tiff" => ("tiff".to_string(), "image/tiff".to_string(), true),
            "image/gif" => ("gif".to_string(), "image/gif".to_string(), true),
            other => (kind.extension().to_string(), other.to_string(), false),
        },
        None => (
            "unknown".to_string(),
            "application/octet-stream".to_string(),
            false,
        ),
    };

    let (width, height) = ImageReader::new(Cursor::new(data))
        .with_guessed_format()
        .ok()
        .and_then(|r| r.into_dimensions().ok())
        .unwrap_or((0, 0));

    let exif_orientation = read_exif_orientation(data);
    let (effective_width, effective_height) = if is_rotated_by_exif(exif_orientation) {
        (height, width)
    } else {
        (width, height)
    };

    let (has_exif, has_icc_profile) = DynImage::from_bytes(Bytes::copy_from_slice(data))
        .ok()
        .flatten()
        .map(|d| (d.exif().is_some(), d.icc_profile().is_some()))
        .unwrap_or((exif_orientation != 1, false));

    let mut hasher = Sha256::new();
    hasher.update(data);
    let sha256 = format!("{:x}", hasher.finalize());

    let total_pixels = (effective_width as u64).saturating_mul(effective_height as u64);
    let megapixels = (total_pixels as f64) / 1_000_000.0;

    Ok(ImageInfo {
        path: path_label.to_string(),
        file_name: file_name.to_string(),
        format,
        mime,
        width,
        height,
        exif_orientation,
        effective_width,
        effective_height,
        total_pixels,
        megapixels: (megapixels * 100.0).round() / 100.0,
        size_bytes,
        human_size: format_human_size(size_bytes as i64),
        has_exif,
        has_icc_profile,
        sha256,
        supported,
        warnings,
    })
}
