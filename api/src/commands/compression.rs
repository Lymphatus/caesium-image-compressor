use crate::compressor::{compress_cimage, preview_cimage, CompressionResult, CompressionStatus, CompressionSummary, OptionsPayload};
use crate::{AppData, CImage, ImageStatus};
use rayon::iter::{IntoParallelRefIterator, ParallelIterator};
use std::cmp::max;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Mutex;
use tauri::{Emitter, Manager};
use std::time::Instant;
use std::sync::Arc;
use std::sync::atomic::AtomicU64;

#[tauri::command]
pub async fn compress(
    app: tauri::AppHandle,
    options: OptionsPayload,
    threads: usize,
    base_folder: String,
) {
    let start_time = Instant::now();
    let total_images = Arc::new(AtomicUsize::new(0));
    let total_success = Arc::new(AtomicUsize::new(0));
    let total_errors = Arc::new(AtomicUsize::new(0));
    let total_warnings = Arc::new(AtomicUsize::new(0));
    let original_size = Arc::new(AtomicUsize::new(0));
    let compressed_size = Arc::new(AtomicUsize::new(0));

    let thread_pool = rayon::ThreadPoolBuilder::new()
        .num_threads(max(threads, 1))
        .build()
        .unwrap(); //TODO

    thread_pool.install(|| {
        //TODO avoid cloning everything if performance will suffer
        let state = app.state::<Mutex<AppData>>();
        let state = state.lock().unwrap();
        // SNAPSHOT what's needed to work on
        let images: Vec<CImage> = state.file_list.iter().cloned().collect();
        //
        drop(state); // Unlock immediately

        total_images.store(images.len(), Ordering::Relaxed);

        let progress = AtomicUsize::new(0);
        app.emit(
            "fileList:compressionProgress",
            progress.load(Ordering::Relaxed),
        )
        .unwrap(); //TODO

        images.par_iter().for_each(|cimage| {
            original_size.fetch_add(cimage.size as usize, Ordering::Relaxed);
            let r = CompressionResult {
                status: CompressionStatus::Warning,
                cimage: CImage {
                    status: ImageStatus::Compressing,
                    ..cimage.clone()
                },
            };
            app.emit("fileList:updateCImage", r).unwrap(); //TODO
            let result = compress_cimage(&app, cimage, &options, &base_folder);
            
            // Count results
            match result.status {
                CompressionStatus::Success => total_success.fetch_add(1, Ordering::Relaxed),
                CompressionStatus::Warning => total_warnings.fetch_add(1, Ordering::Relaxed),
                CompressionStatus::Error => total_errors.fetch_add(1, Ordering::Relaxed),
            };

            compressed_size.fetch_add(result.cimage.compressed_size as usize, Ordering::Relaxed);
            
            let state = app.state::<Mutex<AppData>>();
            let mut state = state.lock().unwrap(); //TODO
            state.file_list.replace(result.clone().cimage);
            app.emit("fileList:updateCImage", result).unwrap(); //TODO
            progress.fetch_add(1, Ordering::Relaxed);
            app.emit(
                "fileList:compressionProgress",
                progress.load(Ordering::Relaxed),
            )
            .unwrap(); //TODO
        });
    });

    let elapsed_time = start_time.elapsed();
    let summary = CompressionSummary {
        total_images: total_images.load(Ordering::Relaxed),
        total_success: total_success.load(Ordering::Relaxed),
        total_skipped: total_warnings.load(Ordering::Relaxed),
        total_errors: total_errors.load(Ordering::Relaxed),
        original_size: original_size.load(Ordering::Relaxed),
        compressed_size: compressed_size.load(Ordering::Relaxed),
        total_time: elapsed_time.as_millis() as u64,
    };

    app.emit("fileList:compressionFinished", summary).unwrap();
}

#[tauri::command]
pub async fn preview(
    app: tauri::AppHandle,
    ids: Vec<String>,
    options: OptionsPayload,
    threads: usize,
) {
    rayon::ThreadPoolBuilder::new().num_threads(max(threads, 1));
    let state = app.state::<Mutex<AppData>>();
    let state = state.lock().unwrap(); //TODO

    let images: Vec<CImage> = ids
        .iter()
        .map(|id| state.file_list.get(id.as_str()).cloned().unwrap())
        .collect();

    drop(state);

    images.par_iter().for_each(|cimage| {
        let result = preview_cimage(&app, cimage, &options);
        let state = app.state::<Mutex<AppData>>();
        let mut state = state.lock().unwrap(); //TODO
        state.file_list.replace(result.clone().cimage);
        app.emit("fileList:updateCImage", result).unwrap(); //TODO
    });
}