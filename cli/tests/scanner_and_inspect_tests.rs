mod common;

use caesium_cli::inspect::inspect_path;
use caesium_cli::options::{ImportFilters, SizeCondition, SizeFilter};
use caesium_cli::scanner::{common_root_folder, load_paths_from_list, scan_inputs, ScanOptions};
use image::ImageFormat;
use std::path::PathBuf;
use tempfile::TempDir;

#[test]
fn computes_common_root_folder_matching_cpp_importer_tests() {
    assert_eq!(
        common_root_folder(&[PathBuf::from("/")]),
        PathBuf::from("/")
    );
    assert_eq!(
        common_root_folder(&[PathBuf::from("/1"), PathBuf::from("/1/1-1")]),
        PathBuf::from("/1")
    );
    assert_eq!(
        common_root_folder(&[PathBuf::from("/1/1-1"), PathBuf::from("/1/1-2")]),
        PathBuf::from("/1")
    );
    assert_eq!(
        common_root_folder(&[
            PathBuf::from("/photos/2026/a/img1.jpg"),
            PathBuf::from("/photos/2026/b/img2.png"),
        ]),
        PathBuf::from("/photos/2026")
    );
}

#[test]
fn scans_directories_recursively_and_non_recursively() {
    let dir = TempDir::new().unwrap();
    common::save_fixture(dir.path(), "root.jpg", 64, 64, ImageFormat::Jpeg);
    common::save_fixture(dir.path(), "sub/nested.png", 64, 64, ImageFormat::Png);
    common::save_fixture(dir.path(), "sub/deep/pic.webp", 64, 64, ImageFormat::WebP);
    std::fs::write(dir.path().join("notes.txt"), "not an image").unwrap();

    let recursive = scan_inputs(
        &[dir.path().to_path_buf()],
        &ScanOptions {
            recursive: true,
            filters: ImportFilters::default(),
            max_input_bytes: Some(500 * 1024 * 1024),
        },
    )
    .unwrap();
    assert_eq!(recursive.files.len(), 3);

    let non_recursive = scan_inputs(
        &[dir.path().to_path_buf()],
        &ScanOptions {
            recursive: false,
            filters: ImportFilters::default(),
            max_input_bytes: Some(500 * 1024 * 1024),
        },
    )
    .unwrap();
    assert_eq!(non_recursive.files.len(), 1);
    assert!(non_recursive.files[0].ends_with("root.jpg"));
}

#[test]
fn applies_size_and_regex_import_filters() {
    let dir = TempDir::new().unwrap();
    let small = common::save_fixture(dir.path(), "thumb_01.png", 16, 16, ImageFormat::Png);
    let large = common::save_fixture(dir.path(), "hero_02.png", 256, 256, ImageFormat::Png);
    let small_len = std::fs::metadata(&small).unwrap().len();
    let large_len = std::fs::metadata(&large).unwrap().len();
    assert!(large_len > small_len);

    // Regex filter: only hero_*
    let by_regex = scan_inputs(
        &[dir.path().to_path_buf()],
        &ScanOptions {
            recursive: true,
            filters: ImportFilters {
                filename_regex: Some(r"^hero_.*\.png$".to_string()),
                ..Default::default()
            },
            max_input_bytes: None,
        },
    )
    .unwrap();
    assert_eq!(by_regex.files.len(), 1);
    assert!(by_regex.files[0].ends_with("hero_02.png"));

    // Skip-by-size filter: skip files less than threshold (GUI parity: "Do not import if size is < X")
    let threshold = (small_len + large_len) / 2;
    let by_size = scan_inputs(
        &[dir.path().to_path_buf()],
        &ScanOptions {
            recursive: true,
            filters: ImportFilters {
                skip_by_size: Some(SizeFilter {
                    condition: SizeCondition::LessThan,
                    bytes: threshold,
                }),
                ..Default::default()
            },
            max_input_bytes: None,
        },
    )
    .unwrap();
    assert_eq!(by_size.files.len(), 1);
    assert!(by_size.files[0].ends_with("hero_02.png"));
}

#[test]
fn loads_file_list_and_deduplicates() {
    let dir = TempDir::new().unwrap();
    let a = common::save_fixture(dir.path(), "a.jpg", 32, 32, ImageFormat::Jpeg);
    let b = common::save_fixture(dir.path(), "b.png", 32, 32, ImageFormat::Png);
    let list_file = dir.path().join("list.txt");
    std::fs::write(
        &list_file,
        format!(
            "# comment\n{}\n\n{}\n{}\n/nonexistent/file.jpg\n",
            a.display(),
            b.display(),
            a.display()
        ),
    )
    .unwrap();

    let paths = load_paths_from_list(&list_file).unwrap();
    assert_eq!(paths.len(), 2);
}

#[test]
fn inspects_image_metadata_dimensions_format_and_hash() {
    let dir = TempDir::new().unwrap();
    let img_path = common::save_fixture(dir.path(), "sample.png", 120, 80, ImageFormat::Png);

    let info = inspect_path(&img_path).unwrap();
    assert_eq!(info.format, "png");
    assert_eq!(info.mime, "image/png");
    assert_eq!(info.width, 120);
    assert_eq!(info.height, 80);
    assert_eq!(info.effective_width, 120);
    assert_eq!(info.effective_height, 80);
    assert!(info.size_bytes > 0);
    assert_eq!(info.sha256.len(), 64);
    assert!(info.supported);
}
