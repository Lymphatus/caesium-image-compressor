import { Select, SelectItem, Slider } from '@heroui/react';
import { useTranslation } from 'react-i18next';
import useCompressionOptionsStore from '@/stores/compression-options.store.ts';
import { TIFF_COMPRESSION_METHOD, TIFF_DEFLATE_LEVEL } from '@/types.ts';

function TiffOptions() {
  const { t } = useTranslation();

  const { tiffOptions, setTiffOptions } = useCompressionOptionsStore();

  const compressionMethods = [
    { key: TIFF_COMPRESSION_METHOD.NONE, label: t('compression_options.tiff.none') },
    { key: TIFF_COMPRESSION_METHOD.DEFLATE, label: t('compression_options.tiff.deflate') },
    { key: TIFF_COMPRESSION_METHOD.LZW, label: t('compression_options.tiff.lzw') },
    { key: TIFF_COMPRESSION_METHOD.PACKBITS, label: t('compression_options.tiff.packbits') },
  ];

  const handleChange = (value: number | number[]) => {
    if (Array.isArray(value)) {
      value = value[0];
    }

    setTiffOptions({ deflateLevel: value });
  };

  return (
    <div className="flex flex-col gap-2">
      <Select
        disallowEmptySelection
        classNames={{
          label: 'text-md',
          trigger: 'shadow-none',
        }}
        label={t('compression_options.tiff.compression_method')}
        labelPlacement="outside"
        selectedKeys={[tiffOptions.method]}
        selectionMode="single"
        size="sm"
        variant="faded"
        onSelectionChange={(value) =>
          setTiffOptions({ method: (value.currentKey as TIFF_COMPRESSION_METHOD) || TIFF_COMPRESSION_METHOD.DEFLATE })
        }
      >
        {compressionMethods.map((cm) => (
          <SelectItem key={cm.key}>{cm.label}</SelectItem>
        ))}
      </Select>

      <Slider
        hideValue
        classNames={{
          label: 'text-sm',
        }}
        isDisabled={tiffOptions.method !== TIFF_COMPRESSION_METHOD.DEFLATE}
        label={t('compression_options.tiff.deflate_level')}
        marks={[
          { value: TIFF_DEFLATE_LEVEL.FAST, label: t('compression_options.tiff.fast') },
          { value: TIFF_DEFLATE_LEVEL.BALANCED, label: t('compression_options.tiff.balanced') },
          { value: TIFF_DEFLATE_LEVEL.BEST, label: t('compression_options.tiff.best') },
        ]}
        maxValue={2}
        minValue={0}
        size="sm"
        step={1}
        value={tiffOptions.deflateLevel}
        onChange={handleChange}
      />
    </div>
  );
}

export default TiffOptions;
