mod common;

use caesium_cli::engine::{
    compress_bytes, dry_run_batch, preview_file, retry_from_receipt, run_batch, JobReceipt,
    OutputTarget,
};
use caesium_cli::error::ErrorCode;
use caesium_cli::options::{
    CompressionMode, CompressionOptions, KeepDatesOptions, MaxOutputSize, MaxOutputSizeUnit,
    MoveOriginalAction, OutputFormat, ResizeMode, ResizeOptions,
};
use image::ImageFormat;
use std::fs::{FileTimes, OpenOptions};
use std::time::{Duration, UNIX_EPOCH};
use tempfile::TempDir;

#[test]
fn compresses_jpeg_png_webp_tiff_and_gif_to_output_dir() {
    let dir = TempDir::new().unwrap();
    let in_dir = dir.path().join("in");
    let out_dir = dir.path().join("out");

    let jpg = common::save_fixture(&in_dir, "photo.jpg", 200, 150, ImageFormat::Jpeg);
    let png = common::save_fixture(&in_dir, "icon.png", 128, 128, ImageFormat::Png);
    let webp = common::save_fixture(&in_dir, "hero.webp", 160, 120, ImageFormat::WebP);
    let tiff = common::save_fixture(&in_dir, "scan.tiff", 100, 100, ImageFormat::Tiff);
    let gif = common::save_fixture(&in_dir, "anim.gif", 80, 80, ImageFormat::Gif);

    let mut opts = CompressionOptions::default();
    opts.jpeg_quality = 60;
    opts.png_quality = 65;
    opts.webp_quality = 55;
    opts.skip_if_bigger = false;

    let report = run_batch(
        &[jpg, png, webp, tiff, gif],
        OutputTarget::Directory(out_dir.clone()),
        &opts,
        2,
        None,
    )
    .unwrap();

    assert_eq!(report.summary.total, 5);
    assert_eq!(report.summary.failed, 0, "items: {:?}", report.items);
    assert!(out_dir.join("photo.jpg").exists());
    assert!(out_dir.join("icon.png").exists());
    assert!(out_dir.join("hero.webp").exists());
    assert!(out_dir.join("scan.tiff").exists());
    assert!(out_dir.join("anim.gif").exists());
}

#[test]
fn compresses_to_target_max_size() {
    let dir = TempDir::new().unwrap();
    let in_file = common::save_fixture(dir.path(), "big.jpg", 512, 512, ImageFormat::Jpeg);
    let orig_size = std::fs::metadata(&in_file).unwrap().len();
    let target_bytes = (orig_size / 3).max(4096) as usize;
    let out_file = dir.path().join("small.jpg");

    let mut opts = CompressionOptions::default();
    opts.mode = CompressionMode::Size;
    opts.max_output_size = MaxOutputSize {
        value: target_bytes as u64,
        unit: MaxOutputSizeUnit::Bytes,
    };
    opts.skip_if_bigger = false;

    let report = run_batch(
        &[in_file],
        OutputTarget::SingleFile(out_file.clone()),
        &opts,
        1,
        None,
    )
    .unwrap();

    assert_eq!(report.summary.failed, 0);
    let compressed_len = std::fs::metadata(&out_file).unwrap().len();
    assert!(
        compressed_len <= orig_size,
        "expected compressed ({}) <= original ({})",
        compressed_len,
        orig_size
    );
}

#[test]
fn resizes_with_all_modes_and_respects_do_not_enlarge() {
    let dir = TempDir::new().unwrap();
    let input = common::save_fixture(dir.path(), "rect.jpg", 400, 200, ImageFormat::Jpeg);

    // Long edge 200 -> 200x100
    let out_long = dir.path().join("long.jpg");
    let mut opts = CompressionOptions::default();
    opts.skip_if_bigger = false;
    opts.resize = ResizeOptions {
        mode: ResizeMode::LongEdge,
        edge: 200,
        do_not_enlarge: true,
        ..Default::default()
    };
    run_batch(
        std::slice::from_ref(&input),
        OutputTarget::SingleFile(out_long.clone()),
        &opts,
        1,
        None,
    )
    .unwrap();
    let dim = image::image_dimensions(&out_long).unwrap();
    assert_eq!(dim, (200, 100));

    // Percentage 50% -> 200x100
    let out_pct = dir.path().join("pct.jpg");
    opts.resize = ResizeOptions {
        mode: ResizeMode::Percentage,
        width: 50,
        height: 50,
        do_not_enlarge: true,
        ..Default::default()
    };
    run_batch(
        std::slice::from_ref(&input),
        OutputTarget::SingleFile(out_pct.clone()),
        &opts,
        1,
        None,
    )
    .unwrap();
    assert_eq!(image::image_dimensions(&out_pct).unwrap(), (200, 100));

    // Do not enlarge: requesting width 800 on 400px image keeps 400x200
    let out_no_enlarge = dir.path().join("no_enlarge.jpg");
    opts.resize = ResizeOptions {
        mode: ResizeMode::FixedWidth,
        width: 800,
        do_not_enlarge: true,
        ..Default::default()
    };
    run_batch(
        std::slice::from_ref(&input),
        OutputTarget::SingleFile(out_no_enlarge.clone()),
        &opts,
        1,
        None,
    )
    .unwrap();
    assert_eq!(image::image_dimensions(&out_no_enlarge).unwrap(), (400, 200));
}

#[test]
fn converts_format_and_preserves_directory_structure_and_file_dates() {
    let dir = TempDir::new().unwrap();
    let in_root = dir.path().join("in");
    let out_root = dir.path().join("out");
    let nested = common::save_fixture(&in_root, "a/b/rgba.png", 120, 90, ImageFormat::Png);

    // Pin input mtime to a known past timestamp
    let pinned = UNIX_EPOCH + Duration::from_secs(1_700_000_000);
    let f = OpenOptions::new().write(true).open(&nested).unwrap();
    f.set_times(FileTimes::new().set_modified(pinned).set_accessed(pinned))
        .unwrap();

    let mut opts = CompressionOptions::default();
    opts.format = OutputFormat::WebP;
    opts.webp_quality = 75;
    opts.keep_structure = true;
    opts.skip_if_bigger = false;
    opts.keep_dates = KeepDatesOptions {
        creation: false,
        modified: true,
        accessed: true,
    };

    let report = run_batch(
        &[nested],
        OutputTarget::Directory(out_root.clone()),
        &opts,
        1,
        Some(in_root),
    )
    .unwrap();

    assert_eq!(report.summary.failed, 0);
    let expected = out_root.join("a/b/rgba.webp");
    assert!(expected.exists(), "expected {:?}", expected);
    let out_meta = std::fs::metadata(&expected).unwrap();
    let out_mtime = out_meta
        .modified()
        .unwrap()
        .duration_since(UNIX_EPOCH)
        .unwrap()
        .as_secs();
    assert_eq!(out_mtime, 1_700_000_000);
}

#[test]
fn refuses_in_place_overwrite_unless_explicitly_enabled() {
    let dir = TempDir::new().unwrap();
    let input = common::save_fixture(dir.path(), "orig.jpg", 100, 100, ImageFormat::Jpeg);

    let mut opts = CompressionOptions::default();
    opts.same_folder_as_input = true;
    opts.overwrite = false;

    let err = run_batch(
        std::slice::from_ref(&input),
        OutputTarget::SameFolder,
        &opts,
        1,
        None,
    )
    .unwrap_err();
    assert_eq!(err.code, ErrorCode::OverwriteRefused);

    // With suffix, same-folder works without --overwrite
    opts.suffix = "_min".into();
    opts.skip_if_bigger = false;
    let ok = run_batch(
        std::slice::from_ref(&input),
        OutputTarget::SameFolder,
        &opts,
        1,
        None,
    )
    .unwrap();
    assert_eq!(ok.summary.failed, 0);
    assert!(dir.path().join("orig_min.jpg").exists());
}

#[test]
fn preview_and_dry_run_do_not_mutate_output_directory() {
    let dir = TempDir::new().unwrap();
    let input = common::save_fixture(dir.path(), "sample.jpg", 200, 150, ImageFormat::Jpeg);
    let out_dir = dir.path().join("never_created");

    let mut opts = CompressionOptions::default();
    opts.jpeg_quality = 50;

    let preview = preview_file(&input, &opts).unwrap();
    assert_eq!(preview.original_size, std::fs::metadata(&input).unwrap().len());
    assert!(preview.compressed_size > 0);
    assert!(!out_dir.exists());

    let plan = dry_run_batch(
        &[input],
        OutputTarget::Directory(out_dir.clone()),
        &opts,
        None,
    )
    .unwrap();
    assert_eq!(plan.items.len(), 1);
    assert!(!out_dir.exists());
}

#[test]
fn compresses_and_converts_in_memory_for_stdin_stdout_streaming() {
    let dir = TempDir::new().unwrap();
    let input = common::save_fixture(dir.path(), "stream.png", 120, 120, ImageFormat::Png);
    let raw = std::fs::read(&input).unwrap();

    let mut opts = CompressionOptions::default();
    opts.format = OutputFormat::WebP;
    opts.webp_quality = 70;

    let out = compress_bytes(&raw, &opts).unwrap();
    assert!(!out.bytes.is_empty());
    assert_eq!(out.output_format, "webp");
    let guessed = infer::get(&out.bytes).unwrap();
    assert_eq!(guessed.mime_type(), "image/webp");
}

#[test]
fn move_original_delete_and_receipt_retry_failed_work() {
    let dir = TempDir::new().unwrap();
    let out_dir = dir.path().join("out");
    let valid = common::save_fixture(dir.path(), "ok.jpg", 120, 120, ImageFormat::Jpeg);
    let broken = dir.path().join("broken.jpg");
    std::fs::write(&broken, b"not a valid jpeg").unwrap();

    let mut opts = CompressionOptions::default();
    opts.skip_if_bigger = false;
    opts.move_original = MoveOriginalAction::Delete;

    let report = run_batch(
        &[valid.clone(), broken.clone()],
        OutputTarget::Directory(out_dir.clone()),
        &opts,
        1,
        None,
    )
    .unwrap();

    assert_eq!(report.summary.succeeded, 1);
    assert_eq!(report.summary.failed, 1);
    // Valid original deleted after successful compression; broken original untouched!
    assert!(!valid.exists());
    assert!(broken.exists());

    // Save receipt, fix broken file, retry ONLY failed items
    let receipt_path = dir.path().join("receipt.json");
    let receipt = JobReceipt::from_report(&report, OutputTarget::Directory(out_dir.clone()), &opts);
    receipt.save(&receipt_path).unwrap();

    // Now replace broken.jpg with a valid image
    common::save_fixture(dir.path(), "broken.jpg", 100, 100, ImageFormat::Jpeg);
    let retry_report = retry_from_receipt(&receipt_path, 1).unwrap();
    assert_eq!(retry_report.summary.total, 1);
    assert_eq!(retry_report.summary.succeeded, 1);
    assert_eq!(retry_report.summary.failed, 0);
    assert!(out_dir.join("broken.jpg").exists());
}
