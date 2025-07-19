import { create } from 'zustand/index';
import { load } from '@tauri-apps/plugin-store';
import { DIRECT_IMPORT_ACTION, POST_COMPRESSION_ACTION, THEME } from '@/types.ts';
import { subscribeWithSelector } from 'zustand/middleware';
import { app, path } from '@tauri-apps/api';
import { invoke } from '@tauri-apps/api/core';
import { platform } from '@tauri-apps/plugin-os';

interface SettingsOptionsStore {
  theme: THEME;
  language: string;
  checkUpdatesAtStartup: boolean;
  promptBeforeExit: boolean;
  skipMessagesAndDialogs: boolean;
  sendUsageData: boolean;
  importSubfolderOnInput: boolean;
  directImportAction: DIRECT_IMPORT_ACTION;
  postCompressionAction: POST_COMPRESSION_ACTION;
  threadsCount: number;
  threadsPriority: number;
  maxThreads: number;

  setTheme: (theme: THEME) => void;
  setLanguage: (language: string) => void;
  setCheckUpdatesAtStartup: (checkUpdatesAtStartup: boolean) => void;
  setPromptBeforeExit: (promptBeforeExit: boolean) => void;
  setSkipMessagesAndDialogs: (skipMessagesAndDialogs: boolean) => void;
  setSendUsageData: (sendUsageData: boolean) => void;
  setImportSubfolderOnInput: (importSubfolderOnInput: boolean) => void;
  setDirectImportAction: (directImportAction: DIRECT_IMPORT_ACTION) => void;
  setPostCompressionAction: (postCompressionAction: POST_COMPRESSION_ACTION) => void;
  setThreadsCount: (threadsCount: number) => void;
  setThreadsPriority: (threadsPriority: number) => void;
}

let configPath = 'settings.json';
if (platform() === 'windows') {
  const exeDir = await invoke<string>('get_executable_dir');
  configPath = await path.join(exeDir, 'settings.json');
}

const settings = await load(configPath, { autoSave: true });
const preferences = (await settings.get('settings')) || {};
const maxThreads = (await invoke<number>('get_max_threads')) || 1;

const defaultOptions = {
  theme: THEME.SYSTEM,
  language: 'en', // TODO
  checkUpdatesAtStartup: true,
  promptBeforeExit: false,
  skipMessagesAndDialogs: false,
  sendUsageData: true,
  importSubfolderOnInput: true,
  directImportAction: DIRECT_IMPORT_ACTION.IMPORT,
  postCompressionAction: POST_COMPRESSION_ACTION.NONE,
  threadsCount: maxThreads,
  threadsPriority: 4,
};

const useSettingsStore = create<SettingsOptionsStore>()(
  subscribeWithSelector((set) => ({
    ...defaultOptions,
    ...preferences,
    maxThreads: maxThreads,
    setTheme: (theme: THEME) => set({ theme }),
    setLanguage: (language: string) => set({ language }),
    setCheckUpdatesAtStartup: (checkUpdatesAtStartup: boolean) => set({ checkUpdatesAtStartup }),
    setPromptBeforeExit: (promptBeforeExit: boolean) => set({ promptBeforeExit }),
    setSkipMessagesAndDialogs: (skipMessagesAndDialogs: boolean) => set({ skipMessagesAndDialogs }),
    setSendUsageData: (sendUsageData: boolean) => set({ sendUsageData }),
    setImportSubfolderOnInput: (importSubfolderOnInput: boolean) => set({ importSubfolderOnInput }),
    setDirectImportAction: (directImportAction: DIRECT_IMPORT_ACTION) => set({ directImportAction }),
    setPostCompressionAction: (postCompressionAction: POST_COMPRESSION_ACTION) => set({ postCompressionAction }),
    setThreadsCount: (threadsCount: number) => set({ threadsCount }),
    setThreadsPriority: (threadsPriority: number) => set({ threadsPriority }),
  })),
);

useSettingsStore.subscribe(async (state) => {
  const dataToSave = {
    theme: state.theme,
    language: state.language,
    checkUpdatesAtStartup: state.checkUpdatesAtStartup,
    promptBeforeExit: state.promptBeforeExit,
    skipMessagesAndDialogs: state.skipMessagesAndDialogs,
    sendUsageData: state.sendUsageData,
    importSubfolderOnInput: state.importSubfolderOnInput,
    directImportAction: state.directImportAction,
    postCompressionAction: state.postCompressionAction,
    threadsCount: state.threadsCount,
    threadsPriority: state.threadsPriority,
  };

  await settings.set('settings', dataToSave);
});

useSettingsStore.subscribe(
  (state) => state.theme,
  async (theme: THEME) => {
    if (
      theme === THEME.LIGHT ||
      (theme === THEME.SYSTEM && !window.matchMedia('(prefers-color-scheme: dark)').matches)
    ) {
      window.document.documentElement.classList.add('light');
      window.document.documentElement.classList.remove('dark');
      await app.setTheme('light');
    } else if (
      theme === THEME.DARK ||
      (theme === THEME.SYSTEM && window.matchMedia('(prefers-color-scheme: dark)').matches)
    ) {
      window.document.documentElement.classList.add('dark');
      window.document.documentElement.classList.remove('light');
      await app.setTheme('dark');
    }
  },
);

export default useSettingsStore;
