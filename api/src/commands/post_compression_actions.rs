use system_shutdown::{shutdown, sleep};

#[derive(Clone, Debug)]
pub enum PostCompressionAction {
    None,
    CloseApp,
    Shutdown,
    Sleep,
    OpenOutputFolder,
}
#[tauri::command]
pub fn exec_post_compression_action(
    _app: tauri::AppHandle,
    post_compression_action: String,
    payload: Option<String>,
) {
    let action = match post_compression_action.as_str() {
        "close_app" => PostCompressionAction::CloseApp,
        "shutdown" => PostCompressionAction::Shutdown,
        "sleep" => PostCompressionAction::Sleep,
        "open_output_folder" => PostCompressionAction::OpenOutputFolder,
        _ => PostCompressionAction::None,
    };

    match action {
        PostCompressionAction::Shutdown => exec_shutdown(),
        PostCompressionAction::Sleep => exec_sleep(),
        PostCompressionAction::OpenOutputFolder => exec_open_output_folder(payload),
        _ => (), // Others are handled by either frontend or ignored
    }
}

fn exec_shutdown() {
    match shutdown() {
        Ok(_) => println!("Shutting down, bye!"),
        Err(error) => eprintln!("Failed to shut down: {}", error),
    }
}

fn exec_sleep() {
    match sleep() {
        Ok(_) => println!("Sleeping, bye!"),
        Err(error) => eprintln!("Failed to sleep: {}", error),
    }
}

fn exec_open_output_folder(output_folder: Option<String>) {
    if let Some(folder) = output_folder {
        open::that_detached(folder).unwrap()
    }
}
