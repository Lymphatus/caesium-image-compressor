import { create } from 'zustand';
import { immer } from 'zustand/middleware/immer';
import { CHROMA_SUBSAMPLING, COMPRESSION_MODE, TIFF_COMPRESSION_METHOD, TIFF_DEFLATE_LEVEL } from '@/types.ts';
import { subscribeWithSelector } from 'zustand/middleware';
import { load } from '@tauri-apps/plugin-store';
import { invoke } from '@tauri-apps/api/core';
import { path } from '@tauri-apps/api';
import { platform } from '@tauri-apps/plugin-os';

interface JpegOptions {
  quality: number;
  chromaSubsampling: CHROMA_SUBSAMPLING;
  progressive: boolean;
}

interface PngOptions {
  quality: number;
  optimizationLevel: number;
}

interface WebpOptions {
  quality: number;
}

interface TiffOptions {
  method: TIFF_COMPRESSION_METHOD;
  deflateLevel: TIFF_DEFLATE_LEVEL;
}

export interface CompressionOptions {
  jpeg: {
    quality: number;
    chroma_subsampling: CHROMA_SUBSAMPLING;
    progressive: boolean;
  };
  png: {
    quality: number;
    optimization_level: number;
  };
  webp: {
    quality: number;
  };
  tiff: {
    method: TIFF_COMPRESSION_METHOD;
    deflate_level: TIFF_DEFLATE_LEVEL;
  };
  compression_mode: COMPRESSION_MODE;
  keep_metadata: boolean;
  lossless: boolean;
  max_size_value: number;
  max_size_unit: number;
}

interface CompressionOptionsStore {
  jpegOptions: JpegOptions;
  pngOptions: PngOptions;
  webpOptions: WebpOptions;
  tiffOptions: TiffOptions;
  lossless: boolean;
  keepMetadata: boolean;
  maxSize: number;
  maxSizeUnit: number;

  setJpegOptions: (options: Partial<JpegOptions>) => void;
  setPngOptions: (options: Partial<PngOptions>) => void;
  setWebpOptions: (options: Partial<WebpOptions>) => void;
  setTiffOptions: (options: Partial<TiffOptions>) => void;
  setLossless: (lossless: boolean) => void;
  setKeepMetadata: (keepMetadata: boolean) => void;
  setMaxSize: (maxSize: number) => void;
  setMaxSizeUnit: (maxSizeUnit: number) => void;

  getCompressionOptions: () => CompressionOptions;
}

const defaultValues = {
  jpegOptions: {
    quality: 80,
    chromaSubsampling: CHROMA_SUBSAMPLING.AUTO,
    progressive: true,
  },
  pngOptions: {
    quality: 80,
    optimizationLevel: 3,
  },
  webpOptions: {
    quality: 80,
  },
  tiffOptions: {
    method: TIFF_COMPRESSION_METHOD.DEFLATE,
    deflateLevel: TIFF_DEFLATE_LEVEL.BALANCED,
  },
  maxSize: 500,
  maxSizeUnit: 1024,
  lossless: false,
  keepMetadata: true,
};

let configPath = 'settings.json';
if (platform() === 'windows') {
  const exeDir = await invoke<string>('get_executable_dir');
  configPath = await path.join(exeDir, 'settings.json');
}

const settings = await load(configPath, { autoSave: true });
const preferences = (await settings.get('compression_options.compression')) || {};

// Create store with default values first
const useCompressionOptionsStore = create<CompressionOptionsStore>()(
  subscribeWithSelector(
    immer((set, get) => ({
      ...defaultValues,
      ...preferences,
      setJpegOptions: (options: Partial<JpegOptions>) =>
        set((state) => {
          Object.assign(state.jpegOptions, options);
        }),
      setPngOptions: (options: Partial<PngOptions>) =>
        set((state) => {
          Object.assign(state.pngOptions, options);
        }),
      setWebpOptions: (options: Partial<WebpOptions>) =>
        set((state) => {
          Object.assign(state.webpOptions, options);
        }),
      setTiffOptions: (options: Partial<TiffOptions>) =>
        set((state) => {
          Object.assign(state.tiffOptions, options);
        }),
      setLossless: (lossless: boolean) =>
        set((state) => {
          state.lossless = lossless;
        }),
      setKeepMetadata: (keepMetadata: boolean) =>
        set((state) => {
          state.keepMetadata = keepMetadata;
        }),
      setMaxSize: (maxSize: number) =>
        set((state) => {
          state.maxSize = maxSize;
        }),
      setMaxSizeUnit: (maxSizeUnit: number) =>
        set((state) => {
          state.maxSizeUnit = maxSizeUnit;
        }),
      getCompressionOptions: () => ({
        jpeg: {
          quality: get().jpegOptions.quality,
          chroma_subsampling: get().jpegOptions.chromaSubsampling,
          progressive: get().jpegOptions.progressive,
        },
        png: {
          quality: get().pngOptions.quality,
          optimization_level: get().pngOptions.optimizationLevel,
        },
        webp: {
          quality: get().webpOptions.quality,
        },
        tiff: {
          method: get().tiffOptions.method,
          deflate_level: get().tiffOptions.deflateLevel,
        },
        compression_mode: COMPRESSION_MODE.QUALITY, //TODO needs a variable
        keep_metadata: get().keepMetadata,
        lossless: get().lossless,
        max_size_value: get().maxSize,
        max_size_unit: get().maxSizeUnit,
      }),
    })),
  ),
);

useCompressionOptionsStore.subscribe(
  (state) => ({
    jpegOptions: state.jpegOptions,
    pngOptions: state.pngOptions,
    webpOptions: state.webpOptions,
    tiffOptions: state.tiffOptions,
    lossless: state.lossless,
    keepMetadata: state.keepMetadata,
    maxSize: state.maxSize,
    maxSizeUnit: state.maxSizeUnit,
  }),
  async (data) => {
    await settings.set('compression_options.compression', data);
  },
);

export default useCompressionOptionsStore;
