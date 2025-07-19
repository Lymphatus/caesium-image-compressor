// noinspection TypeScriptUMDGlobal

import { ImageLoaderRequest, ImageLoaderResponse } from '@/types.ts';

self.onmessage = async function (event) {
  const eventData = event.data as ImageLoaderRequest;
  if (eventData.mimeType === 'image/tiff') {
    const imageBitmap = await loadTiff(eventData.imageUrl);
    const responseMessage: ImageLoaderResponse = { imageBitmap, type: eventData.type };
    postMessage(responseMessage);
  } else {
    const response = await fetch(eventData.imageUrl);
    const blob = await response.blob();
    const imageBitmap = await createImageBitmap(blob);
    const responseMessage: ImageLoaderResponse = { imageBitmap, type: eventData.type };
    postMessage(responseMessage);
  }
};

async function loadTiff(imageURL: string): Promise<ImageBitmap> {
  return new Promise((resolve, reject) => {
    importScripts('./UTIF.js');
    const xhr = new XMLHttpRequest();
    xhr.open('GET', imageURL);
    xhr.responseType = 'arraybuffer';
    xhr.onload = async function (e: ProgressEvent) {
      if (!e.target) {
        reject(new Error('Failed to load image'));
        return;
      }
      try {
        // @ts-expect-error only way to add this library rn
        const ifds = UTIF.decode((e.target as XMLHttpRequest).response);
        // @ts-expect-error only way to add this library rn
        UTIF.decodeImage((e.target as XMLHttpRequest).response, ifds[0]);
        // @ts-expect-error only way to add this library rn
        const rgba = UTIF.toRGBA8(ifds[0]); // Uint8Array with RGBA pixels
        const imageData = new ImageData(new Uint8ClampedArray(rgba.buffer), ifds[0].width, ifds[0].height);
        const imageBitmap = await createImageBitmap(imageData);
        resolve(imageBitmap);
      } catch (error) {
        reject(error);
      }
    };
    xhr.onerror = () => reject(new Error('Network error'));
    xhr.send();
  });
}
