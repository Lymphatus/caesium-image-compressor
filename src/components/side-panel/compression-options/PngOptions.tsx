import { NumberInput, Slider } from '@heroui/react';
import { useTranslation } from 'react-i18next';
import useCompressionOptionsStore from '@/stores/compression-options.store.ts';

function PngOptions() {
  const { t } = useTranslation();

  const { pngOptions, setPngOptions, lossless } = useCompressionOptionsStore();

  const handleChange = (type: 'quality' | 'optimizationLevel', value: number | number[]) => {
    if (Array.isArray(value)) {
      value = value[0];
    }

    if (type === 'quality') {
      setPngOptions({ quality: value });
    } else if (type === 'optimizationLevel') {
      setPngOptions({ optimizationLevel: value });
    }
  };

  return (
    <div className="flex flex-col gap-2">
      <Slider
        classNames={{
          label: 'text-sm',
        }}
        isDisabled={lossless}
        label={t('quality')}
        maxValue={100}
        minValue={0}
        renderValue={() => (
          <NumberInput
            aria-label={t('quality')}
            className="max-w-20"
            classNames={{
              inputWrapper: 'p-1 h-8 shadow-none',
              input: 'text-right',
            }}
            maxValue={100}
            minValue={0}
            size="sm"
            value={pngOptions.quality}
            variant="faded"
            onValueChange={(value) => setPngOptions({ quality: value })}
          ></NumberInput>
        )}
        size="sm"
        step={1}
        value={pngOptions.quality}
        onChange={(v) => handleChange('quality', v)}
      />

      <Slider
        classNames={{
          label: 'text-sm',
        }}
        isDisabled={!lossless}
        label={t('compression_options.optimization_level')}
        maxValue={6}
        minValue={1}
        renderValue={() => (
          <NumberInput
            aria-label={t('compression_options.optimization_level')}
            className="max-w-20"
            classNames={{
              inputWrapper: 'p-1 h-8 shadow-none',
              input: 'text-right',
            }}
            maxValue={6}
            minValue={1}
            size="sm"
            value={pngOptions.optimizationLevel}
            variant="faded"
            onValueChange={(value) => setPngOptions({ optimizationLevel: value })}
          ></NumberInput>
        )}
        size="sm"
        step={1}
        value={pngOptions.optimizationLevel}
        onChange={(v) => handleChange('optimizationLevel', v)}
      />
    </div>
  );
}

export default PngOptions;
