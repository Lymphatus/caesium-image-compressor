#![allow(dead_code)]

use image::{DynamicImage, ImageBuffer, ImageFormat, Rgb, Rgba};
use std::path::{Path, PathBuf};

/// Creates a deterministic colorful gradient RGB image suitable for real compression testing.
pub fn create_gradient_rgb(width: u32, height: u32) -> DynamicImage {
    let mut img = ImageBuffer::<Rgb<u8>, Vec<u8>>::new(width, height);
    for (x, y, pixel) in img.enumerate_pixels_mut() {
        let r = ((x * 255) / width.max(1)) as u8;
        let g = ((y * 255) / height.max(1)) as u8;
        let b = (((x + y) * 127) / (width + height).max(1)) as u8;
        // Add gentle high-frequency variation so lossy codecs achieve meaningful compression ratios
        let noise = ((x.wrapping_mul(31) ^ y.wrapping_mul(17)) & 0x0f) as u8;
        *pixel = Rgb([r.saturating_add(noise), g, b.saturating_add(noise)]);
    }
    DynamicImage::ImageRgb8(img)
}

/// Creates a deterministic RGBA image with transparency.
pub fn create_gradient_rgba(width: u32, height: u32) -> DynamicImage {
    let mut img = ImageBuffer::<Rgba<u8>, Vec<u8>>::new(width, height);
    for (x, y, pixel) in img.enumerate_pixels_mut() {
        let r = ((x * 255) / width.max(1)) as u8;
        let g = ((y * 255) / height.max(1)) as u8;
        let b = 180u8;
        let a = if (x + y) % 4 == 0 { 128 } else { 255 };
        *pixel = Rgba([r, g, b, a]);
    }
    DynamicImage::ImageRgba8(img)
}

pub fn save_fixture(dir: &Path, name: &str, width: u32, height: u32, format: ImageFormat) -> PathBuf {
    let path = dir.join(name);
    if let Some(parent) = path.parent() {
        std::fs::create_dir_all(parent).expect("create fixture parent dir");
    }
    let img = if matches!(format, ImageFormat::Png | ImageFormat::WebP) {
        create_gradient_rgba(width, height)
    } else {
        create_gradient_rgb(width, height)
    };
    img.save_with_format(&path, format)
        .expect("save synthetic fixture image");
    path
}
