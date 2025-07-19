use crate::{AppData, CImage, ImageStatus};
use file_format::FileFormat;
use serde::Serialize;
use sha2::{Digest, Sha256};
use std::cmp::min;
use std::sync::Mutex;
use std::{
    fs::File,
    io::BufReader,
    path::{absolute, Path, PathBuf},
};
use tauri::{Emitter, Manager};
use tauri_plugin_dialog::FilePath;
use walkdir::WalkDir;

#[derive(serde::Serialize, Clone)]
pub struct FileImportProgress {
    pub(crate) progress: usize,
    pub(crate) total: usize,
}
#[derive(Serialize, Clone)]
pub struct FileList {
    pub(crate) files: Vec<CImage>,
    pub(crate) base_folder: String,
    pub(crate) total_files: usize,
}

fn is_filetype_supported(path: &Path) -> bool {
    let fmt = get_file_mime_type(path);
    let mime_type = fmt.media_type();

    if mime_type == "image/tiff" {
        let extension = path.extension().unwrap().to_ascii_lowercase();
        if extension != "tif" && extension != "tiff" {
            return false;
        }
    }

    matches!(
        mime_type,
        "image/jpeg" | "image/png" | "image/gif" | "image/webp" | "image/tiff"
    )
}

pub fn get_file_mime_type(path: &Path) -> FileFormat {
    FileFormat::from_file(path).unwrap() //TODO
}

fn is_valid(entry: &Path) -> bool {
    entry.exists() && entry.is_file() && is_filetype_supported(entry)
}

pub fn process_files(app: &tauri::AppHandle, file_paths: Vec<FilePath>, recursive: bool) {
    app.emit("fileImporter:importStarted", ()).unwrap(); //TODO
    let state = app.state::<Mutex<AppData>>();
    let mut state = state.lock().unwrap();
    let (base_folder, imported_files) = scan_files(&file_paths, &state.base_path, recursive);

    state.base_path = base_folder;

    app.emit("fileImporter:scanFinished", ()).unwrap(); //TODO

    let total = imported_files.len();
    let mut progress = 0;
    for (index, f) in imported_files.iter().enumerate() {
        let new_progress = ((index + 1) as f64 / total as f64 * 100.0).floor() as usize;

        if progress != new_progress {
            progress = new_progress;
            app.emit(
                "fileImporter:importProgress",
                FileImportProgress { progress, total },
            )
            .unwrap_or_default();
        }
        //TODO What is default doing here?

        let cimage = match map_file(f) {
            Some(c) => c,
            _ => continue,
        };

        state.file_list.insert(cimage);
    }

    let mut full_list = vec![];

    if !state.file_list.is_empty() {
        state
            .file_list
            .sort_by(|a, b| a.path.partial_cmp(&b.path).unwrap()); //TODO

        let offset = (state.current_page - 1) * 50;
        full_list = state
            .file_list
            .get_range(offset..min(state.file_list.len(), 50)) //TODO check out of range
            .unwrap() //TODO
            .iter()
            .cloned()
            .collect();
    }

    app.emit("fileImporter:importFinished", ()).unwrap(); //TODO
    app.emit(
        "fileList:getList",
        FileList {
            files: full_list,
            total_files: state.file_list.len(),
            base_folder: absolute(&state.base_path)
                .unwrap_or_default()
                .to_str()
                .unwrap() //TODO
                .to_string(),
        },
    )
    .unwrap(); //TODO
}

pub fn scan_files(
    args: &Vec<FilePath>,
    initial_base_path: &PathBuf,
    recursive: bool,
) -> (PathBuf, Vec<PathBuf>) {
    if args.is_empty() {
        return (initial_base_path.clone(), vec![]);
    }
    let mut files: Vec<PathBuf> = vec![];
    let mut base_path = initial_base_path.clone();

    for path in args.iter() {
        let input = PathBuf::from(path.as_path().unwrap()); //TODO
        if input.exists() && input.is_dir() {
            let mut walk_dir = WalkDir::new(input);
            if !recursive {
                walk_dir = walk_dir.max_depth(1);
            }
            for entry in walk_dir.into_iter().filter_map(|e| e.ok()) {
                let path = entry.into_path();
                if is_valid(&path) {
                    base_path = match compute_base_path(&path, &base_path) {
                        Some(p) => p,
                        None => continue,
                    };
                    files.push(path);
                }
            }
        } else if is_valid(&input) {
            base_path = match compute_base_path(&input, &base_path) {
                Some(p) => p,
                None => continue,
            };
            files.push(input);
        }
    }

    (base_path, files)
}

pub fn compute_base_path(path: &Path, base_path: &Path) -> Option<PathBuf> {
    if let Ok(ap) = absolute(path) {
        let bp = compute_base_folder(base_path, &ap)?;
        return Some(bp);
    }

    None
}

fn compute_base_folder(base_folder: &Path, new_path: &Path) -> Option<PathBuf> {
    if base_folder.as_os_str().is_empty() && new_path.parent().is_some() {
        return Some(new_path.parent()?.to_path_buf());
    }

    if base_folder.parent().is_none() {
        return Some(base_folder.to_path_buf());
    }

    let mut folder = PathBuf::new();
    let mut new_path_folder = new_path.to_path_buf();
    if new_path.is_file() {
        new_path_folder = new_path
            .parent()
            .unwrap_or(&*PathBuf::from("/"))
            .to_path_buf();
    }
    for (i, component) in base_folder.iter().enumerate() {
        if let Some(new_path_component) = new_path_folder.iter().nth(i) {
            if new_path_component == component {
                folder.push(component);
            } else {
                break;
            }
        } else {
            break;
        }
    }

    if folder.parent().is_none() {
        return Some(folder);
    }

    Some(folder)
}

pub fn map_file(file: &PathBuf) -> Option<CImage> {
    // let id = Uuid::new_v4().to_string();

    let name = file.file_name()?.to_str()?.to_string();
    let directory = file.parent()?.to_str()?.to_string();
    let size = match file.metadata() {
        Ok(m) => m.len(),
        Err(..) => return None,
    };
    let path = file.to_str()?.to_string();
    let id = base16ct::lower::encode_string(&Sha256::digest(path.as_bytes()));

    let mime_type = get_file_mime_type(file);
    let (width, height) = get_real_resolution(file, mime_type.media_type());

    let cimage = CImage {
        id,
        name,
        path,
        directory,
        mime_type: mime_type.media_type().to_string(),
        size,
        width,
        height,
        compressed_width: 0,
        compressed_height: 0,
        compressed_size: 0,
        compressed_file_path: String::new(),
        info: String::new(),
        status: ImageStatus::New,
    };

    Some(cimage)
}

pub fn get_real_resolution(file: &Path, mime_type: &str) -> (usize, usize) {
    let resolution = match imagesize::size(file) {
        Ok(r) => r,
        Err(_) => return (0, 0),
    };
    let mut orientation = 1;
    if mime_type == "image/jpeg" {
        let f = File::open(file).unwrap(); //TODO
        if let Ok(e) = exif::Reader::new().read_from_container(&mut BufReader::new(&f)) {
            let exif_field = match e.get_field(exif::Tag::Orientation, exif::In::PRIMARY) {
                Some(f) => f,
                None => return (resolution.width, resolution.height),
            };
            orientation = exif_field.value.get_uint(0).unwrap_or(1);
        };
    }
    let (width, height) = match orientation {
        5..=8 => (resolution.height, resolution.width),
        _ => (resolution.width, resolution.height),
    };

    (width, height)
}
