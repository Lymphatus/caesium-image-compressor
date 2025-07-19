import { create } from 'zustand/index';
import { load } from '@tauri-apps/plugin-store';
import { subscribeWithSelector } from 'zustand/middleware';
import { path } from '@tauri-apps/api';
import { invoke } from '@tauri-apps/api/core';
import { platform } from '@tauri-apps/plugin-os';
import { v4 as uuidv4 } from 'uuid';

interface AppOptionsStore {
  uuid: string;

  setUuid: (uuid: string) => void;
}

let configPath = 'settings.json';
if (platform() === 'windows') {
  const exeDir = await invoke<string>('get_executable_dir');
  configPath = await path.join(exeDir, 'settings.json');
}

const settings = await load(configPath, { autoSave: true });
const preferences = (await settings.get('app')) || {};

const defaultOptions = {
  uuid: uuidv4(),
};

const useAppStore = create<AppOptionsStore>()(
  subscribeWithSelector((set) => ({
    ...defaultOptions,
    ...preferences,

    setUuid: (uuid: string) => set({ uuid }),
  })),
);

await settings.set('app', {
  uuid: useAppStore.getState().uuid,
});
await settings.save();

export default useAppStore;
