import { CImage, IMAGE_STATUS } from '@/types.ts';
import { create } from 'zustand/index';
import useFileListStore from '@/stores/file-list.store.ts';
import { invoke } from '@tauri-apps/api/core';
import useCompressionOptionsStore from '@/stores/compression-options.store.ts';
import useResizeOptionsStore from '@/stores/resize-options.store.ts';
import useOutputOptionsStore from '@/stores/output-options.store.ts';
import { subscribeWithSelector } from 'zustand/middleware';
import useUIStore from '@/stores/ui.store.ts';
import useSettingsStore from '@/stores/settings.store.ts';

interface PreviewStore {
  isLoading: boolean;
  currentPreviewedCImage: CImage | null;
  visualizationMode: 'original' | 'compressed';

  setIsLoading: (isLoading: boolean) => void;
  setCurrentPreviewedCImage: (cImage: CImage | null) => void;
  setVisualizationMode: (visualizationMode: 'original' | 'compressed') => void;

  getCurrentPreviewedCImage: () => CImage | null;
  invokePreview: (ids: string[]) => void;
}

const usePreviewStore = create<PreviewStore>()(
  subscribeWithSelector((set, get) => ({
    isLoading: false,
    currentPreviewedCImage: null,
    visualizationMode: 'original',

    setIsLoading: (isLoading: boolean) => set({ isLoading }),
    setCurrentPreviewedCImage: (cImage: CImage | null) => set({ currentPreviewedCImage: cImage }),
    setVisualizationMode: (visualizationMode: 'original' | 'compressed') => set({ visualizationMode }),

    getCurrentPreviewedCImage: () => get().currentPreviewedCImage,
    invokePreview: (ids: string[]) => {
      for (const id of ids) {
        useFileListStore.getState().updateFile(id, { status: IMAGE_STATUS.COMPRESSING });
      }
      invoke('preview', {
        ids,
        options: {
          compression_options: useCompressionOptionsStore.getState().getCompressionOptions(),
          resize_options: useResizeOptionsStore.getState().getResizeOptions(),
          output_options: useOutputOptionsStore.getState().getOutputOptions(),
        },
        threads: useSettingsStore.getState().threadsCount,
      }).catch((e) => {
        for (const id of ids) {
          useFileListStore.getState().updateFile(id, { status: IMAGE_STATUS.ERROR, info: e.toString() }); //TODO maybe we don't need to set all of them as errors
        }
      });
    },
  })),
);

usePreviewStore.subscribe(
  (state) => state.currentPreviewedCImage?.id,
  (id) => {
    if (id && useUIStore.getState().autoPreview && useUIStore.getState().showPreviewPanel) {
      usePreviewStore.getState().invokePreview([id]);
    }
  },
);

useFileListStore.subscribe(
  (state) => state.fileList,
  async (fileList) => {
    if (
      fileList.length === 0 ||
      fileList.find((cImage) => cImage.id === usePreviewStore.getState().currentPreviewedCImage?.id) === undefined
    ) {
      usePreviewStore.getState().setCurrentPreviewedCImage(null);
    }
  },
);

export default usePreviewStore;
