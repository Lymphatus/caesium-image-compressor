import { create } from 'zustand/index';
import { immer } from 'zustand/middleware/immer';
import { load } from '@tauri-apps/plugin-store';
import { SIDE_PANEL_TAB } from '@/types.ts';
import { subscribeWithSelector } from 'zustand/middleware';
import { invoke } from '@tauri-apps/api/core';
import { path } from '@tauri-apps/api';
import { platform } from '@tauri-apps/plugin-os';

interface SplitPanels {
  main: number[];
  center: number[];
}

interface UIOptions {
  splitPanels: SplitPanels;
  jpegAccordionOpen: boolean;
  pngAccordionOpen: boolean;
  webpAccordionOpen: boolean;
  tiffAccordionOpen: boolean;
  currentSelectedTab: SIDE_PANEL_TAB;
  settingsDialogOpen: boolean;
  showPreviewPanel: boolean;
  autoPreview: boolean;
  showLabelsInToolbar: boolean;
  aboutDialogOpen: boolean;
  promptExitDialogOpen: boolean;
  checkForUpdatesDialogOpen: boolean;

  setSplitPanels: (options: Partial<SplitPanels>) => void;
  setJpegAccordionOpen: (open: boolean) => void;
  setPngAccordionOpen: (open: boolean) => void;
  setWebpAccordionOpen: (open: boolean) => void;
  setTiffAccordionOpen: (open: boolean) => void;
  setCurrentSelectedTab: (tab: SIDE_PANEL_TAB) => void;
  setSettingsDialogOpen: (open: boolean) => void;
  setShowPreviewPanel: (show: boolean) => void;
  setAutoPreview: (autoPreview: boolean) => void;
  setShowLabelsInToolbar: (show: boolean) => void;
  setAboutDialogOpen: (open: boolean) => void;
  setPromptExitDialogOpen: (open: boolean) => void;
  setCheckForUpdatesDialogOpen: (open: boolean) => void;

  getAppMenuSelectedItems: () => string[];
}

let configPath = 'settings.json';
if (platform() === 'windows') {
  const exeDir = await invoke<string>('get_executable_dir');
  configPath = await path.join(exeDir, 'settings.json');
}

const settings = await load(configPath, { autoSave: true });
const preferences = (await settings.get('ui')) || {};

const defaultOptions = {
  splitPanels: { main: [70, 30], center: [60, 40] },
  jpegAccordionOpen: true,
  pngAccordionOpen: true,
  webpAccordionOpen: true,
  tiffAccordionOpen: true,
  currentSelectedTab: SIDE_PANEL_TAB.COMPRESSION,
  settingsDialogOpen: false,
  showPreviewPanel: true,
  autoPreview: false,
  showLabelsInToolbar: false,
  aboutDialogOpen: false,
  promptExitDialogOpen: false,
  checkForUpdatesDialogOpen: false,
};

const useUIStore = create<UIOptions>()(
  subscribeWithSelector(
    immer((set, get) => ({
      ...defaultOptions,
      ...preferences,
      setSplitPanels: (options: Partial<SplitPanels>) =>
        set((state) => {
          Object.assign(state.splitPanels, options);
        }),

      setJpegAccordionOpen: (open: boolean) => {
        set((state) => {
          state.jpegAccordionOpen = open;
        });
      },
      setPngAccordionOpen: (open: boolean) => {
        set((state) => {
          state.pngAccordionOpen = open;
        });
      },
      setWebpAccordionOpen: (open: boolean) => {
        set((state) => {
          state.webpAccordionOpen = open;
        });
      },
      setTiffAccordionOpen: (open: boolean) => {
        set((state) => {
          state.tiffAccordionOpen = open;
        });
      },
      setCurrentSelectedTab: (tab: SIDE_PANEL_TAB) => {
        set((state) => {
          state.currentSelectedTab = tab;
        });
      },
      setSettingsDialogOpen: (open: boolean) => {
        set((state) => {
          state.settingsDialogOpen = open;
        });
      },
      setShowPreviewPanel: (show: boolean) => {
        set((state) => {
          state.showPreviewPanel = show;
        });
      },
      setAutoPreview: (autoPreview: boolean) => {
        set((state) => {
          state.autoPreview = autoPreview;
        });
      },
      setShowLabelsInToolbar: (show: boolean) => {
        set((state) => {
          state.showLabelsInToolbar = show;
        });
      },

      setAboutDialogOpen: (open: boolean) => {
        set((state) => {
          state.aboutDialogOpen = open;
        });
      },

      setPromptExitDialogOpen: (open: boolean) => {
        set((state) => {
          state.promptExitDialogOpen = open;
        });
      },

      setCheckForUpdatesDialogOpen: (open: boolean) => {
        set((state) => {
          state.checkForUpdatesDialogOpen = open;
        });
      },

      getAppMenuSelectedItems: () => {
        const selectedItems = [];
        if (get().showPreviewPanel) selectedItems.push('showPreview');
        if (get().autoPreview) selectedItems.push('autoPreview');
        if (get().showLabelsInToolbar) selectedItems.push('showToolbarLabels');
        return selectedItems;
      },
    })),
  ),
);

useUIStore.subscribe(async (state) => {
  const dataToSave = {
    splitPanels: state.splitPanels,
    jpegAccordionOpen: state.jpegAccordionOpen,
    pngAccordionOpen: state.pngAccordionOpen,
    webpAccordionOpen: state.webpAccordionOpen,
    tiffAccordionOpen: state.tiffAccordionOpen,
    currentSelectedTab: state.currentSelectedTab,
    showPreviewPanel: state.showPreviewPanel,
    autoPreview: state.autoPreview,
    showLabelsInToolbar: state.showLabelsInToolbar,
  };

  await settings.set('ui', dataToSave);
});

export default useUIStore;
