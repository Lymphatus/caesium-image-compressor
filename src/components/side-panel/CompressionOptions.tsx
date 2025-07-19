import {
  Accordion,
  AccordionItem,
  NumberInput,
  Select,
  SelectItem,
  SharedSelection,
  Switch,
  Tab,
  Tabs,
} from '@heroui/react';
import { useTranslation } from 'react-i18next';
import JpegOptions from '@/components/side-panel/compression-options/JpegOptions.tsx';
import useUIStore from '@/stores/ui.store.ts';
import PngOptions from '@/components/side-panel/compression-options/PngOptions.tsx';
import WebpOptions from '@/components/side-panel/compression-options/WebpOptions.tsx';
import TiffOptions from '@/components/side-panel/compression-options/TiffOptions.tsx';
import useCompressionOptionsStore from '@/stores/compression-options.store.ts';
import type { Selection } from '@react-types/shared';

enum ACCORDION_KEY {
  JPEG = 'jpeg_accordion',
  PNG = 'png_accordion',
  WEBP = 'webp_accordion',
  TIFF = 'tiff_accordion',
}

function CompressionOptions() {
  const { t } = useTranslation();
  const {
    jpegAccordionOpen,
    pngAccordionOpen,
    webpAccordionOpen,
    tiffAccordionOpen,
    setJpegAccordionOpen,
    setPngAccordionOpen,
    setWebpAccordionOpen,
    setTiffAccordionOpen,
  } = useUIStore();

  const { lossless, setLossless, keepMetadata, setKeepMetadata, maxSize, setMaxSize, maxSizeUnit, setMaxSizeUnit } =
    useCompressionOptionsStore();

  const handleChange = (value: SharedSelection) => {
    let actualValue = 1024;
    if (value instanceof Selection) {
      setMaxSizeUnit(actualValue);

      return;
    }

    actualValue = parseInt(value.currentKey || '1024');
    setMaxSizeUnit(actualValue);
  };

  const maxSizeUnits = [
    {
      key: 1,
      label: t('size_units.byte', {
        count: 2,
      }),
    },
    { key: 1024, label: t('size_units.kb') },
    { key: 1024 * 1024, label: t('size_units.mb') },
  ];

  const defaultAccordionOpen: ACCORDION_KEY[] = [];
  if (jpegAccordionOpen) {
    defaultAccordionOpen.push(ACCORDION_KEY.JPEG);
  }
  if (pngAccordionOpen) {
    defaultAccordionOpen.push(ACCORDION_KEY.PNG);
  }
  if (webpAccordionOpen) {
    defaultAccordionOpen.push(ACCORDION_KEY.WEBP);
  }
  if (tiffAccordionOpen) {
    defaultAccordionOpen.push(ACCORDION_KEY.TIFF);
  }

  const handleAccordionOpen = (keys: Selection) => {
    if (keys === 'all') {
      setJpegAccordionOpen(true);
      setPngAccordionOpen(true);
      setWebpAccordionOpen(true);
      setTiffAccordionOpen(true);

      return;
    }

    setJpegAccordionOpen(keys.has(ACCORDION_KEY.JPEG));
    setPngAccordionOpen(keys.has(ACCORDION_KEY.PNG));
    setWebpAccordionOpen(keys.has(ACCORDION_KEY.WEBP));
    setTiffAccordionOpen(keys.has(ACCORDION_KEY.TIFF));
  };

  return (
    <div className="size-full overflow-auto">
      <div className="p-2 text-sm">
        <Tabs fullWidth size="sm">
          <Tab title={t('quality')}>
            <div className="flex flex-col gap-2">
              <Accordion
                isCompact
                keepContentMounted
                className="!px-0"
                defaultSelectedKeys={defaultAccordionOpen}
                itemClasses={{
                  base: 'shadow-none bg-content2',
                  content: 'py-2',
                }}
                selectionMode="multiple"
                variant="splitted"
                onSelectionChange={handleAccordionOpen}
              >
                <AccordionItem key={ACCORDION_KEY.JPEG} aria-label={t('formats.jpeg')} title={t('formats.jpeg')}>
                  <JpegOptions></JpegOptions>
                </AccordionItem>
                <AccordionItem key={ACCORDION_KEY.PNG} aria-label={t('formats.png')} title={t('formats.png')}>
                  <PngOptions></PngOptions>
                </AccordionItem>
                <AccordionItem key={ACCORDION_KEY.WEBP} aria-label={t('formats.webp')} title={t('formats.webp')}>
                  <WebpOptions></WebpOptions>
                </AccordionItem>
                <AccordionItem key={ACCORDION_KEY.TIFF} aria-label={t('formats.tiff')} title={t('formats.tiff')}>
                  <TiffOptions></TiffOptions>
                </AccordionItem>
              </Accordion>

              <div className="flex w-full items-center justify-between">
                <div className="flex flex-col">
                  <span>{t('compression_options.lossless')}</span>
                </div>
                <Switch isSelected={lossless} size="sm" onValueChange={setLossless}></Switch>
              </div>
              <div className="flex w-full items-center justify-between">
                <div className="flex flex-col">
                  <span>{t('compression_options.keep_metadata')}</span>
                </div>
                <Switch isSelected={keepMetadata} size="sm" onValueChange={setKeepMetadata}></Switch>
              </div>
            </div>
          </Tab>
          <Tab title={t('size')}>
            <NumberInput
              endContent={
                <Select
                  disallowEmptySelection
                  aria-label={'units'}
                  className="max-w-[100px] p-1"
                  classNames={{
                    label: 'hidden',
                    trigger: 'shadow-none',
                  }}
                  label={''}
                  selectedKeys={[maxSizeUnit.toString()]}
                  selectionMode="single"
                  size="sm"
                  variant="faded"
                  onSelectionChange={(v) => handleChange(v)}
                >
                  {maxSizeUnits.map((unit) => (
                    <SelectItem key={unit.key}>{unit.label}</SelectItem>
                  ))}
                </Select>
              }
              label={t('compression_options.max_output_size')}
              labelPlacement="outside"
              maxValue={999}
              minValue={1}
              placeholder="500"
              size="sm"
              step={1}
              value={maxSize}
              variant="faded"
              onValueChange={(v) => setMaxSize(v)}
            />
          </Tab>
        </Tabs>
      </div>
    </div>
  );
}

export default CompressionOptions;
