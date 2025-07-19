import { RESIZE_MODE } from '@/types.ts';
import { NumberInput, Select, SelectItem, Switch } from '@heroui/react';
import { useTranslation } from 'react-i18next';
import useResizeOptionsStore from '@/stores/resize-options.store.ts';

function ResizeOptions() {
  const { t } = useTranslation();
  const {
    resizeMode,
    width,
    height,
    widthPercentage,
    heightPercentage,
    dimension,
    keepAspectRatio,
    doNotEnlarge,
    setResizeMode,
    setWidth,
    setHeight,
    setWidthPercentage,
    setHeightPercentage,
    setDimension,
    setKeepAspectRatio,
    setDoNotEnlarge,
  } = useResizeOptionsStore();

  const resizeModes = [
    { key: RESIZE_MODE.NONE, label: t('resize_modes.none') },
    {
      key: RESIZE_MODE.DIMENSIONS,
      label: t('resize_modes.dimensions'),
    },
    {
      key: RESIZE_MODE.PERCENTAGE,
      label: t('resize_modes.percentage'),
    },
    {
      key: RESIZE_MODE.WIDTH,
      label: t('resize_modes.width'),
    },
    {
      key: RESIZE_MODE.HEIGHT,
      label: t('resize_modes.height'),
    },
    {
      key: RESIZE_MODE.LONG_EDGE,
      label: t('resize_modes.long_edge'),
    },
    {
      key: RESIZE_MODE.SHORT_EDGE,
      label: t('resize_modes.short_edge'),
    },
  ];

  const showPixelDimensionsInputs = [RESIZE_MODE.DIMENSIONS, RESIZE_MODE.WIDTH, RESIZE_MODE.HEIGHT].includes(
    resizeMode,
  );
  const showPercentageInputs = resizeMode === RESIZE_MODE.PERCENTAGE;
  const showDimensionInput = [RESIZE_MODE.LONG_EDGE, RESIZE_MODE.SHORT_EDGE].includes(resizeMode);
  const showResizeControls = resizeMode !== RESIZE_MODE.NONE;
  const keepAspectRatioDisabled = [
    RESIZE_MODE.DIMENSIONS,
    RESIZE_MODE.WIDTH,
    RESIZE_MODE.HEIGHT,
    RESIZE_MODE.LONG_EDGE,
    RESIZE_MODE.SHORT_EDGE,
  ].includes(resizeMode);

  let dimensionLabel = '';
  switch (resizeMode) {
    case RESIZE_MODE.LONG_EDGE:
      dimensionLabel = t('resize_modes.long_edge');
      break;
    case RESIZE_MODE.SHORT_EDGE:
      dimensionLabel = t('resize_modes.short_edge');
      break;
    case RESIZE_MODE.WIDTH:
      dimensionLabel = t('resize_modes.width');
      break;
    case RESIZE_MODE.HEIGHT:
      dimensionLabel = t('resize_modes.height');
      break;
    default:
      break;
  }

  return (
    <div className="size-full overflow-auto">
      <div className="flex flex-col gap-2 p-2 text-sm">
        <Select
          disallowEmptySelection
          classNames={{
            label: 'text-md',
            trigger: 'shadow-none',
          }}
          label={t('compression_options.resize_options.resize_mode')}
          labelPlacement="outside"
          selectedKeys={[resizeMode]}
          selectionMode="single"
          size="sm"
          variant="faded"
          onSelectionChange={(value) => setResizeMode((value.currentKey as RESIZE_MODE) || RESIZE_MODE.NONE)}
        >
          {resizeModes.map((r) => (
            <SelectItem key={r.key}>{r.label}</SelectItem>
          ))}
        </Select>
        {showResizeControls && (
          <>
            {showPixelDimensionsInputs && (
              <div className="flex w-full items-center justify-between gap-2">
                <NumberInput
                  disableAnimation
                  aria-label={t('width')}
                  className="min-w-32"
                  classNames={{
                    inputWrapper: 'p-1 h-8 shadow-none',
                    input: 'text-right',
                  }}
                  endContent={
                    <div className="pointer-events-none flex items-center">
                      <span className="text-default-400 text-small">px</span>
                    </div>
                  }
                  isDisabled={resizeMode === RESIZE_MODE.HEIGHT}
                  label={t('width')}
                  labelPlacement="outside"
                  maxValue={99999}
                  minValue={1}
                  size="sm"
                  value={width}
                  variant="faded"
                  onValueChange={(value) => setWidth(value)}
                ></NumberInput>
                <NumberInput
                  disableAnimation
                  aria-label={t('height')}
                  className="min-w-32"
                  classNames={{
                    inputWrapper: 'p-1 h-8 shadow-none',
                    input: 'text-right',
                  }}
                  endContent={
                    <div className="pointer-events-none flex items-center">
                      <span className="text-default-400 text-small">px</span>
                    </div>
                  }
                  isDisabled={resizeMode === RESIZE_MODE.WIDTH}
                  label={t('height')}
                  labelPlacement="outside"
                  maxValue={99999}
                  minValue={1}
                  size="sm"
                  value={height}
                  variant="faded"
                  onValueChange={(value) => setHeight(value)}
                ></NumberInput>
              </div>
            )}
            {showPercentageInputs && (
              <div className="flex w-full items-center justify-between gap-2">
                <NumberInput
                  disableAnimation
                  aria-label={t('width')}
                  className="min-w-32"
                  classNames={{
                    inputWrapper: 'p-1 h-8 shadow-none',
                    input: 'text-right',
                  }}
                  endContent={
                    <div className="pointer-events-none flex items-center">
                      <span className="text-default-400 text-small">%</span>
                    </div>
                  }
                  label={t('width')}
                  labelPlacement="outside"
                  maxValue={doNotEnlarge ? 100 : 999}
                  minValue={1}
                  size="sm"
                  value={widthPercentage}
                  variant="faded"
                  onValueChange={(value) => {
                    setWidthPercentage(value);
                    if (keepAspectRatio) {
                      setHeightPercentage(value);
                    }
                  }}
                ></NumberInput>
                <NumberInput
                  disableAnimation
                  aria-label={t('height')}
                  className="min-w-32"
                  classNames={{
                    inputWrapper: 'p-1 h-8 shadow-none',
                    input: 'text-right',
                  }}
                  endContent={
                    <div className="pointer-events-none flex items-center">
                      <span className="text-default-400 text-small">%</span>
                    </div>
                  }
                  label={t('height')}
                  labelPlacement="outside"
                  maxValue={doNotEnlarge ? 100 : 999}
                  minValue={1}
                  size="sm"
                  value={heightPercentage}
                  variant="faded"
                  onValueChange={(value) => {
                    setHeightPercentage(value);
                    if (keepAspectRatio) {
                      setWidthPercentage(value);
                    }
                  }}
                ></NumberInput>
              </div>
            )}
            {showDimensionInput && (
              <NumberInput
                disableAnimation
                aria-label={dimensionLabel}
                className="min-w-32"
                classNames={{
                  inputWrapper: 'p-1 h-8 shadow-none',
                  input: 'text-right',
                }}
                endContent={
                  <div className="pointer-events-none flex items-center">
                    <span className="text-default-400 text-small">px</span>
                  </div>
                }
                label={dimensionLabel}
                labelPlacement="outside"
                maxValue={99999}
                minValue={1}
                size="sm"
                value={dimension}
                variant="faded"
                onValueChange={(value) => setDimension(value)}
              ></NumberInput>
            )}
            <div className="flex w-full items-center justify-between">
              <div className="flex flex-col">
                <span>{t('compression_options.resize_options.do_not_enlarge')}</span>
              </div>
              <Switch
                isSelected={doNotEnlarge}
                size="sm"
                onValueChange={(enabled) => {
                  setDoNotEnlarge(enabled);
                  if (enabled) {
                    if (widthPercentage > 100) {
                      setWidthPercentage(100);
                    }
                    if (heightPercentage > 100) {
                      setHeightPercentage(100);
                    }
                  }
                }}
              ></Switch>
            </div>

            <div className="flex w-full items-center justify-between">
              <div className="flex flex-col">
                <span>{t('compression_options.resize_options.keep_aspect_ratio')}</span>
              </div>
              <Switch
                isDisabled={keepAspectRatioDisabled}
                isSelected={keepAspectRatio}
                size="sm"
                onValueChange={(enabled) => {
                  setKeepAspectRatio(enabled);
                  if (enabled) {
                    setHeightPercentage(widthPercentage);
                  }
                }}
              ></Switch>
            </div>
          </>
        )}
      </div>
    </div>
  );
}

export default ResizeOptions;
