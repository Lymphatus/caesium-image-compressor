use crate::scan_files::get_real_resolution;
use crate::{CImage, ImageStatus};
use caesium::parameters::{CSParameters, ChromaSubsampling, TiffCompression, TiffDeflateLevel};
use caesium::{
    compress, compress_in_memory, compress_to_size, compress_to_size_in_memory, convert_in_memory,
    SupportedFileTypes,
};
use serde_json::to_string;
use sha2::{Digest, Sha256};
use std::ffi::OsString;
use std::fs::{copy, File, FileTimes, Metadata};
use std::io::{Read, Write};
#[cfg(any(windows, doc))]
use std::os::windows::fs::FileTimesExt;
use std::path::{absolute, Path, PathBuf};
use std::{fs, io};
use tauri::path::BaseDirectory;
use tauri::Manager;

#[derive(serde::Serialize, serde::Deserialize, Clone, Debug)]
struct JPEGOptions {
    quality: u32,
    chroma_subsampling: String, //TODO Create type
    progressive: bool,
}
#[derive(serde::Serialize, serde::Deserialize, Clone, Debug)]
struct PNGOptions {
    quality: u32,
    optimization_level: u32,
}
#[derive(serde::Serialize, serde::Deserialize, Clone, Debug)]
struct WebPOptions {
    quality: u32,
}
#[derive(serde::Serialize, serde::Deserialize, Clone, Debug)]
struct TIFFOptions {
    method: String, //TODO Create type
    deflate_level: u32,
}

#[derive(serde::Serialize, serde::Deserialize, Clone, Debug)]
struct CompressionOptions {
    jpeg: JPEGOptions,
    png: PNGOptions,
    webp: WebPOptions,
    tiff: TIFFOptions,
    compression_mode: u32, //TODO Create type
    keep_metadata: bool,
    lossless: bool,
    max_size_value: usize,
    max_size_unit: usize,
}

#[derive(serde::Serialize, serde::Deserialize, Clone, Debug)]
struct ResizeOptions {
    resize_enabled: bool,
    resize_mode: String, //TODO Create type
    keep_aspect_ratio: bool,
    do_not_enlarge: bool,
    width: u32,
    height: u32,
    width_percentage: u32,
    height_percentage: u32,
    long_edge: u32,
    short_edge: u32,
}

#[derive(serde::Serialize, serde::Deserialize, Clone, Debug)]
struct OutputOptions {
    output_folder: String,
    same_folder_as_input: bool,
    keep_folder_structure: bool,
    skip_if_output_is_bigger: bool,
    move_original_file_enabled: bool,
    move_original_file_mode: String, // TODO Create type
    keep_file_dates_enabled: bool,
    keep_creation_date: bool,
    keep_last_modified_date: bool,
    keep_last_access_date: bool,
    output_format: String, //TODO Create type
    suffix: String,
}

#[derive(serde::Serialize, serde::Deserialize, Clone, Debug)]
pub struct OptionsPayload {
    compression_options: CompressionOptions,
    resize_options: ResizeOptions,
    output_options: OutputOptions,
}

#[derive(serde::Serialize, serde::Deserialize, Clone, Debug)]
pub struct CompressionResult {
    pub status: CompressionStatus,
    pub cimage: CImage,
}

#[derive(serde::Serialize, serde::Deserialize, Clone, Debug)]
pub enum CompressionStatus {
    Success,
    Warning,
    Error,
}

#[derive()]
enum ResizeMode {
    None,
    Dimensions,
    Width,
    Height,
    LongEdge,
    ShortEdge,
    Percentage,
}

const MAX_FILE_SIZE: u64 = 500 * 1024 * 1024;

pub fn compress_cimage(
    _app: &tauri::AppHandle,
    cimage: &CImage,
    options: &OptionsPayload,
    base_folder: &str,
) -> CompressionResult {
    let original_file_size = cimage.size;

    if original_file_size > MAX_FILE_SIZE {
        return CompressionResult {
            status: CompressionStatus::Error,
            cimage: CImage {
                status: ImageStatus::Error,
                info: format!("File exceeds max size of {MAX_FILE_SIZE}").to_string(),
                ..cimage.clone()
            },
        };
    }

    let output_full_path = match setup_output_path(cimage.path.as_ref(), options, base_folder) {
        Some(path) => path,
        None => {
            return CompressionResult {
                status: CompressionStatus::Error,
                cimage: CImage {
                    status: ImageStatus::Error,
                    info: "Error computing output path".to_string(),
                    ..cimage.clone()
                },
            }
        }
    };

    let mut compression_parameters = parse_compression_options(options, cimage);

    if options.resize_options.do_not_enlarge {
        if compression_parameters.width > cimage.width as u32
            || compression_parameters.height > cimage.height as u32
        {
            return CompressionResult {
                status: CompressionStatus::Warning,
                cimage: CImage {
                    status: ImageStatus::Warning,
                    info: "Cannot resize over original dimensions, skipping".to_string(),
                    compressed_width: cimage.width,
                    compressed_height: cimage.height,
                    compressed_size: cimage.size,
                    compressed_file_path: output_full_path.display().to_string(),
                    ..cimage.clone()
                },
            };
        }
    }

    let compressed_image =
        match perform_image_compression(cimage, options, &mut compression_parameters) {
            Some(image) => image,
            None => {
                return CompressionResult {
                    status: CompressionStatus::Error,
                    cimage: CImage {
                        status: ImageStatus::Error,
                        info: "Error while compressing".to_string(),
                        ..cimage.clone()
                    },
                }
            }
        };

    let mut new_width = cimage.width;
    let mut new_height = cimage.height;
    if options.resize_options.resize_enabled {
        (new_width, new_height) = get_real_resolution(&output_full_path, cimage.mime_type.as_str());
    }

    let output_file_size = compressed_image.len() as u64;
    let output_file_exists = output_full_path.exists();

    if original_file_size < output_file_size && options.output_options.skip_if_output_is_bigger {
        if PathBuf::from(&cimage.path) != output_full_path {
            copy(&cimage.path, &output_full_path).unwrap();
        }

        return CompressionResult {
            status: CompressionStatus::Warning,
            cimage: CImage {
                status: ImageStatus::Warning,
                info: "Compressed file is bigger, skipping".to_string(),
                compressed_width: cimage.width,
                compressed_height: cimage.height,
                compressed_size: cimage.size,
                compressed_file_path: output_full_path.display().to_string(),
                ..cimage.clone()
            },
        };
    }

    let mut output_file = File::create(&output_full_path).unwrap(); //TODO

    output_file.write_all(&compressed_image).unwrap(); //TODO

    if options.output_options.keep_file_dates_enabled {
        let input_metadata = PathBuf::from(cimage.path.clone()).metadata().unwrap(); //TODO

        preserve_file_times(&output_file, &input_metadata, options).unwrap(); //TODO
    }

    if options.output_options.move_original_file_enabled
        && options.output_options.move_original_file_mode == "trash"
    {
        trash::delete(&cimage.path).unwrap();
    }

    CompressionResult {
        status: CompressionStatus::Success,
        cimage: CImage {
            compressed_width: new_width,
            compressed_height: new_height,
            compressed_size: output_file_size,
            compressed_file_path: output_full_path.display().to_string(),
            info: String::new(),
            status: ImageStatus::Success,
            ..cimage.clone()
        },
    }
}

// TODO I don't like using the payload here
pub fn preview_cimage(
    app: &tauri::AppHandle,
    cimage: &CImage,
    options: &OptionsPayload,
) -> CompressionResult {
    let filename = options_payload_to_sha256(&cimage.id, options);
    let mut parameters = parse_compression_options(options, cimage);
    let output_path = app.path().resolve(filename, BaseDirectory::Temp).unwrap(); //TODO

    if options.resize_options.do_not_enlarge {
        if parameters.width > cimage.width as u32 || parameters.height > cimage.height as u32 {
            return CompressionResult {
                status: CompressionStatus::Warning,
                cimage: CImage {
                    status: ImageStatus::Warning,
                    info: "Cannot resize over original dimensions, skipping".to_string(),
                    ..cimage.clone()
                },
            };
        }
    }

    let result = if options.compression_options.compression_mode == 1 {
        let output_size =
            options.compression_options.max_size_value * options.compression_options.max_size_unit;
        compress_to_size(
            cimage.path.clone(),
            output_path.display().to_string(),
            &mut parameters,
            output_size,
            true,
        )
        .is_ok()
    } else {
        compress(
            cimage.path.clone(),
            output_path.display().to_string(),
            &parameters,
        )
        .is_ok()
    };

    if !result {
        return CompressionResult {
            status: CompressionStatus::Error,
            cimage: CImage {
                status: ImageStatus::Error,
                ..cimage.clone()
            },
        };
    }
    let output_path = Path::new(&output_path);
    let size = fs::metadata(output_path).unwrap().len(); //TODO
    let mut new_width = cimage.width;
    let mut new_height = cimage.height;
    if options.resize_options.resize_enabled {
        (new_width, new_height) = get_real_resolution(output_path, cimage.mime_type.as_str());
    }

    CompressionResult {
        status: CompressionStatus::Success,
        cimage: CImage {
            compressed_width: new_width,
            compressed_height: new_height,
            compressed_size: size,
            compressed_file_path: output_path.display().to_string(),
            info: String::new(),
            status: ImageStatus::Success,
            ..cimage.clone()
        },
    }
}

fn parse_compression_options(options: &OptionsPayload, cimage: &CImage) -> CSParameters {
    let mut parameters = CSParameters::new();

    parameters.keep_metadata = options.compression_options.keep_metadata;
    parameters.optimize = options.compression_options.lossless;

    // TODO use an enum or a match
    if options.resize_options.resize_enabled {
        let resize_mode = match options.resize_options.resize_mode.as_str() {
            "none" => ResizeMode::None,
            "dimensions" => ResizeMode::Dimensions,
            "width" => ResizeMode::Width,
            "height" => ResizeMode::Height,
            "long_edge" => ResizeMode::LongEdge,
            "short_edge" => ResizeMode::ShortEdge,
            "percentage" => ResizeMode::Percentage,
            _ => ResizeMode::None,
        };
        if matches!(resize_mode, ResizeMode::Dimensions) {
            parameters.width = options.resize_options.width;
            parameters.height = options.resize_options.height;
        } else if matches!(resize_mode, ResizeMode::Percentage) {
            parameters.width =
                f64::from(options.resize_options.width_percentage * cimage.width as u32 / 100)
                    .round() as u32;
            parameters.height =
                f64::from(options.resize_options.height_percentage * cimage.height as u32 / 100)
                    .round() as u32;
        } else if matches!(resize_mode, ResizeMode::ShortEdge) {
            if cimage.width > cimage.height {
                parameters.height = options.resize_options.short_edge;
                parameters.width = 0;
            } else {
                parameters.width = options.resize_options.short_edge;
                parameters.height = 0;
            }
        } else if matches!(resize_mode, ResizeMode::LongEdge) {
            if cimage.width > cimage.height {
                parameters.width = options.resize_options.long_edge;
                parameters.height = 0;
            } else {
                parameters.height = options.resize_options.long_edge;
                parameters.width = 0;
            }
        } else if matches!(resize_mode, ResizeMode::Width) {
            parameters.width = options.resize_options.width;
            parameters.height = 0;
        } else if matches!(resize_mode, ResizeMode::Height) {
            parameters.width = 0;
            parameters.height = options.resize_options.height;
        }
    }

    // -- JPEG --
    parameters.jpeg.quality = options.compression_options.jpeg.quality;
    parameters.jpeg.chroma_subsampling =
        parse_jpeg_chroma_subsampling(options.compression_options.jpeg.chroma_subsampling.as_str());

    // -- PNG --
    parameters.png.quality = options.compression_options.png.quality;
    parameters.png.optimization_level = options.compression_options.png.optimization_level as u8;
    parameters.png.force_zopfli = false;

    // -- WEBP --
    parameters.webp.quality = options.compression_options.webp.quality;

    // -- TIFF --
    parameters.tiff.algorithm = match options.compression_options.tiff.method.as_str() {
        "lzw" => TiffCompression::Lzw,
        "deflate" => TiffCompression::Deflate,
        "packbits" => TiffCompression::Packbits,
        _ => TiffCompression::Uncompressed,
    };
    parameters.tiff.deflate_level = match options.compression_options.tiff.deflate_level {
        1 => TiffDeflateLevel::Fast,
        9 => TiffDeflateLevel::Best,
        _ => TiffDeflateLevel::Balanced,
    };

    parameters
}

fn options_payload_to_sha256(id: &String, options: &OptionsPayload) -> String {
    // Serialize the struct to a JSON string
    let json_string = to_string(options).unwrap(); //TODO

    // Compute the SHA256 hash
    let mut hasher = Sha256::new();
    hasher.update(id);
    hasher.update("|");
    hasher.update(json_string);
    let result = hasher.finalize();

    // Convert the hash to a hexadecimal string
    format!("{result:x}")
}

fn perform_image_compression(
    cimage: &CImage,
    options: &OptionsPayload,
    compression_parameters: &mut CSParameters,
) -> Option<Vec<u8>> {
    let mut file = File::open(cimage.path.clone()).unwrap(); //TODO
    let mut input_file_buffer = Vec::new();
    file.read_to_end(&mut input_file_buffer).unwrap(); //TODO

    let compression_result_data = if options.compression_options.compression_mode == 1 {
        //SIZE
        compress_to_size_in_memory(
            input_file_buffer,
            compression_parameters,
            options.compression_options.max_size_value * options.compression_options.max_size_unit,
            true,
        )
    } else if options.compression_options.compression_mode == 1
        && options.output_options.output_format != "original"
    {
        convert_in_memory(
            input_file_buffer,
            &compression_parameters,
            map_supported_formats(&options.output_options.output_format),
        )
    } else {
        compress_in_memory(input_file_buffer, compression_parameters)
    };

    compression_result_data.ok()
}
fn setup_output_path(
    input_file: &Path,
    options: &OptionsPayload,
    base_folder: &str,
) -> Option<PathBuf> {
    let output_directory = determine_output_directory(input_file, options)?;
    let (output_directory, filename) = compute_output_full_path(
        &output_directory,
        input_file,
        &PathBuf::from(base_folder),
        options.output_options.keep_folder_structure,
        &options.output_options.suffix,
        &options.output_options.output_format,
    )?;

    if !output_directory.exists() && fs::create_dir_all(&output_directory).is_err() {
        return None; // TODO track errors here
    }

    Some(output_directory.join(filename))
}

fn determine_output_directory(input_file: &Path, options: &OptionsPayload) -> Option<PathBuf> {
    if options.output_options.same_folder_as_input {
        input_file.parent().map(|p| p.to_path_buf())
    } else {
        let folder = options.output_options.output_folder.clone();
        Some(PathBuf::from(folder))
    }
}

fn compute_output_full_path(
    output_directory: &Path,
    input_file_path: &Path,
    base_directory: &PathBuf,
    keep_structure: bool,
    suffix: &str,
    format: &str,
) -> Option<(PathBuf, OsString)> {
    let extension = if format == "original" {
        input_file_path
            .extension()
            .unwrap_or_default()
            .to_os_string()
    } else {
        OsString::from(format)
    };

    let base_name = input_file_path
        .file_stem()
        .unwrap_or_default()
        .to_os_string();
    let mut output_file_name = base_name;
    output_file_name.push(suffix);
    if !extension.is_empty() {
        output_file_name.push(".");
        output_file_name.push(extension);
    }

    if keep_structure {
        let parent = match absolute(input_file_path.parent()?) {
            Ok(p) => p,
            Err(_) => return None,
        };

        let output_path_prefix = match parent.strip_prefix(base_directory) {
            Ok(p) => p,
            Err(_) => return None,
        };
        let full_output_directory = output_directory.join(output_path_prefix);
        Some((full_output_directory, output_file_name))
    } else {
        Some((PathBuf::from(output_directory), output_file_name))
    }
}

fn parse_jpeg_chroma_subsampling(chroma_subsampling: &str) -> ChromaSubsampling {
    match chroma_subsampling {
        "4:4:4" => ChromaSubsampling::CS444,
        "4:2:2" => ChromaSubsampling::CS422,
        "4:2:0" => ChromaSubsampling::CS420,
        "4:1:1" => ChromaSubsampling::CS411,
        _ => ChromaSubsampling::Auto,
    }
}

fn map_supported_formats(format: &str) -> SupportedFileTypes {
    match format {
        "jpg" => SupportedFileTypes::Jpeg,
        "png" => SupportedFileTypes::Png,
        "webp" => SupportedFileTypes::WebP,
        "tiff" => SupportedFileTypes::Tiff,
        _ => SupportedFileTypes::Unkn,
    }
}

fn preserve_file_times(
    output_file: &File,
    original_file_metadata: &Metadata,
    options: &OptionsPayload,
) -> io::Result<()> {
    let mut file_times = FileTimes::new();

    #[cfg(target_os = "windows")]
    {
        if options.output_options.keep_creation_date {
            let created_time = original_file_metadata.created()?;
            file_times = file_times.set_created(created_time);
        }
    }

    if options.output_options.keep_last_modified_date {
        let modified_time = original_file_metadata.modified()?;
        file_times = file_times.set_modified(modified_time);
    }

    if options.output_options.keep_last_access_date {
        let accessed_time = original_file_metadata.accessed()?;
        file_times = file_times.set_accessed(accessed_time);
    }

    output_file.set_times(file_times)?;
    Ok(())
}
