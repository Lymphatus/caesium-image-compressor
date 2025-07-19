import { POST_COMPRESSION_ACTION } from '@/types.ts';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { invoke } from '@tauri-apps/api/core';
import useOutputOptionsStore from '@/stores/output-options.store.ts';

export async function execPostCompressionAction(postCompressionAction: POST_COMPRESSION_ACTION) {
  if (postCompressionAction === POST_COMPRESSION_ACTION.CLOSE_APP) {
    await closeApp();
  } else if (postCompressionAction === POST_COMPRESSION_ACTION.SHUTDOWN) {
    await shutdown();
  } else if (postCompressionAction === POST_COMPRESSION_ACTION.SLEEP) {
    await sleep();
  } else if (postCompressionAction === POST_COMPRESSION_ACTION.OPEN_OUTPUT_FOLDER) {
    await openOutputFolder();
  }
}

async function closeApp() {
  await getCurrentWindow().destroy();
}

async function shutdown() {
  await invoke('exec_post_compression_action', {
    postCompressionAction: POST_COMPRESSION_ACTION.SHUTDOWN,
  });
}

async function sleep() {
  await invoke('exec_post_compression_action', {
    postCompressionAction: POST_COMPRESSION_ACTION.SLEEP,
  });
}

async function openOutputFolder() {
  await invoke('exec_post_compression_action', {
    postCompressionAction: POST_COMPRESSION_ACTION.OPEN_OUTPUT_FOLDER,
    payload: useOutputOptionsStore.getState().outputFolder,
  });
}
