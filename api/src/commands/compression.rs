use crate::compressor::{
    compress_cimage, preview_cimage, CompressionResult, CompressionStatus, OptionsPayload,
};
use crate::{AppData, CImage, ImageStatus};
use rayon::iter::{IntoParallelRefIterator, ParallelIterator};
use std::cmp::max;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Mutex;
use tauri::{Emitter, Manager};

#[tauri::command]
pub async fn compress(
    app: tauri::AppHandle,
    options: OptionsPayload,
    threads: usize,
    base_folder: String,
) {
    let thread_pool = rayon::ThreadPoolBuilder::new()
        .num_threads(max(threads, 1))
        .build()
        .unwrap(); //TODO

    thread_pool.install(|| {
        //TODO avoid cloning everything if performance will suffer
        let state = app.state::<Mutex<AppData>>();
        let state = state.lock().unwrap();
        // // SNAPSHOT what's needed to work on
        let images: Vec<CImage> = state.file_list.iter().cloned().collect();
        //
        drop(state); // Unlock immediately

        let progress = AtomicUsize::new(0);
        app.emit(
            "fileList:compressionProgress",
            progress.load(Ordering::Relaxed),
        )
        .unwrap(); //TODO

        images.par_iter().for_each(|cimage| {
            let r = CompressionResult {
                status: CompressionStatus::Warning,
                cimage: CImage {
                    status: ImageStatus::Compressing,
                    ..cimage.clone()
                },
            };
            app.emit("fileList:updateCImage", r).unwrap(); //TODO
            let result = compress_cimage(&app, cimage, &options, &base_folder);
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
