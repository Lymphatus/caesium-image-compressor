import { create } from 'zustand/index';
import { load } from '@tauri-apps/plugin-store';
import { FILE_DATE, MOVE_ORIGINAL_FILE, OUTPUT_FORMAT } from '@/types.ts';
import { invoke } from '@tauri-apps/api/core';
import { path } from '@tauri-apps/api';
import { platform } from '@tauri-apps/plugin-os';

interface OutputOptionsStore {
  outputFolder: string;
  sameFolderAsInput: boolean;
  keepFolderStructure: boolean;
  skipIfOutputIsBigger: boolean;
  moveOriginalFile: boolean;
  moveOriginalFileType: MOVE_ORIGINAL_FILE;
  keepFileDates: FILE_DATE[];
  outputFormat: OUTPUT_FORMAT;
  suffix: string;

  setOutputFolder: (outputFolder: string) => void;
  setSameFolderAsInput: (sameFolderAsInput: boolean) => void;
  setKeepFolderStructure: (keepFolderStructure: boolean) => void;
  setSkipIfOutputIsBigger: (skipIfOutputIsBigger: boolean) => void;
  setMoveOriginalFile: (moveOriginalFile: boolean) => void;
  setMoveOriginalFileType: (moveOriginalFileType: MOVE_ORIGINAL_FILE) => void;
  setKeepFileDates: (keepFileDates: FILE_DATE[]) => void;
  setOutputFormat: (outputFormat: OUTPUT_FORMAT) => void;
  setSuffix: (suffix: string) => void;

  getOutputOptions: () => OutputOptions;
}

export interface OutputOptions {
  output_folder: string;
  same_folder_as_input: boolean;
  keep_folder_structure: boolean;
  skip_if_output_is_bigger: boolean;
  move_original_file_enabled: boolean;
  move_original_file_mode: MOVE_ORIGINAL_FILE;
  keep_file_dates_enabled: boolean;
  keep_creation_date: boolean;
  keep_last_modified_date: boolean;
  keep_last_access_date: boolean;
  output_format: OUTPUT_FORMAT;
  suffix: string;
}

let configPath = 'settings.json';
if (platform() === 'windows') {
  const exeDir = await invoke<string>('get_executable_dir');
  configPath = await path.join(exeDir, 'settings.json');
}

const settings = await load(configPath, { autoSave: true });
const preferences = (await settings.get('compression_options.output')) || {};

const defaultOptions = {
  outputFolder: '',
  sameFolderAsInput: false,
  keepFolderStructure: true,
  skipIfOutputIsBigger: true,
  moveOriginalFile: false,
  moveOriginalFileType: MOVE_ORIGINAL_FILE.TRASH,
  keepFileDates: [FILE_DATE.MODIFIED, FILE_DATE.CREATED, FILE_DATE.ACCESSED],
  outputFormat: OUTPUT_FORMAT.ORIGINAL,
  suffix: '',
};

const useOutputOptionsStore = create<OutputOptionsStore>()((set, get) => ({
  ...defaultOptions,
  ...preferences,

  setOutputFolder: (outputFolder: string) => set({ outputFolder }),
  setSameFolderAsInput: (sameFolderAsInput: boolean) => set({ sameFolderAsInput }),
  setKeepFolderStructure: (keepFolderStructure: boolean) => set({ keepFolderStructure }),
  setSkipIfOutputIsBigger: (skipIfOutputIsBigger: boolean) => set({ skipIfOutputIsBigger }),
  setMoveOriginalFile: (moveOriginalFile: boolean) => set({ moveOriginalFile }),
  setMoveOriginalFileType: (moveOriginalFileType: MOVE_ORIGINAL_FILE) => set({ moveOriginalFileType }),
  setKeepFileDates: (keepFileDates: FILE_DATE[]) => set({ keepFileDates }),
  setOutputFormat: (outputFormat: OUTPUT_FORMAT) => set({ outputFormat }),
  setSuffix: (suffix: string) => set({ suffix }),

  getOutputOptions: () => ({
    output_folder: get().outputFolder,
    same_folder_as_input: get().sameFolderAsInput,
    keep_folder_structure: get().keepFolderStructure,
    skip_if_output_is_bigger: get().skipIfOutputIsBigger,
    move_original_file_enabled: get().moveOriginalFile,
    move_original_file_mode: get().moveOriginalFileType,
    keep_file_dates_enabled: get().keepFileDates.length > 0,
    keep_creation_date: get().keepFileDates.includes(FILE_DATE.CREATED),
    keep_last_modified_date: get().keepFileDates.includes(FILE_DATE.MODIFIED),
    keep_last_access_date: get().keepFileDates.includes(FILE_DATE.ACCESSED),
    output_format: get().outputFormat,
    suffix: get().suffix,
  }),
}));

useOutputOptionsStore.subscribe(async (state) => {
  const dataToSave = {
    outputFolder: state.outputFolder,
    sameFolderAsInput: state.sameFolderAsInput,
    keepFolderStructure: state.keepFolderStructure,
    skipIfOutputIsBigger: state.skipIfOutputIsBigger,
    moveOriginalFile: state.moveOriginalFile,
    moveOriginalFileType: state.moveOriginalFileType,
    keepFileDates: state.keepFileDates,
    outputFormat: state.outputFormat,
    suffix: state.suffix,
  };

  await settings.set('compression_options.output', dataToSave);
});

export default useOutputOptionsStore;
