import { Button, Checkbox, Divider, Input, Select, SelectItem, Switch } from '@heroui/react';
import { useTranslation } from 'react-i18next';
import useOutputOptionsStore from '@/stores/output-options.store.ts';
import { open } from '@tauri-apps/plugin-dialog';
import { FILE_DATE, MOVE_ORIGINAL_FILE, OUTPUT_FORMAT } from '@/types.ts';

function OutputOptions() {
  const { t } = useTranslation();

  const {
    outputFolder,
    sameFolderAsInput,
    keepFolderStructure,
    skipIfOutputIsBigger,
    moveOriginalFile,
    moveOriginalFileType,
    keepFileDates,
    outputFormat,
    suffix,
    setKeepFolderStructure,
    setSkipIfOutputIsBigger,
    setSameFolderAsInput,
    setOutputFolder,
    setMoveOriginalFile,
    setMoveOriginalFileType,
    setKeepFileDates,
    setOutputFormat,
    setSuffix,
  } = useOutputOptionsStore();

  const toggleFileDateCreation = (type: FILE_DATE, enabled: boolean) => {
    const selection = [...keepFileDates];
    if (enabled && !selection.includes(type)) {
      selection.push(type);
    } else if (!enabled && selection.includes(type)) {
      selection.splice(selection.indexOf(type), 1);
    }

    setKeepFileDates(selection);
  };

  const moveOriginalFileTypes = [
    { key: MOVE_ORIGINAL_FILE.TRASH, label: t('move_original_files_modes.trash') },
    { key: MOVE_ORIGINAL_FILE.DELETE, label: t('move_original_files_modes.delete') },
  ];
  const outputFormats = [
    { key: OUTPUT_FORMAT.ORIGINAL, label: t('formats.original') },
    { key: OUTPUT_FORMAT.JPEG, label: t('formats.jpeg') },
    { key: OUTPUT_FORMAT.PNG, label: t('formats.png') },
    { key: OUTPUT_FORMAT.WEBP, label: t('formats.webp') },
    { key: OUTPUT_FORMAT.TIFF, label: t('formats.tiff') },
  ];

  return (
    <div className="size-full overflow-auto">
      <div className="flex flex-col gap-2 p-2 text-sm">
        <div className="flex w-full flex-col gap-1">
          <Input
            readOnly
            classNames={{
              inputWrapper: 'pr-0 shadow-none',
            }}
            endContent={
              <Button
                disableRipple
                className="h-full rounded-l-none"
                size="sm"
                onPress={async () => {
                  const folder = await open({ directory: true, multiple: false });
                  if (folder) {
                    setOutputFolder(folder);
                  }
                }}
              >
                {t('select_dots')}
              </Button>
            }
            isDisabled={sameFolderAsInput}
            label={t('compression_options.output_folder')}
            labelPlacement="outside"
            placeholder={t('compression_options.output_folder')}
            size="sm"
            value={outputFolder}
            variant="faded"
          />
          <div className="flex w-full items-center justify-between">
            <div className="flex flex-col">
              <span>{t('compression_options.output_options.same_folder_as_input')}</span>
            </div>
            <Switch isSelected={sameFolderAsInput} size="sm" onValueChange={setSameFolderAsInput}></Switch>
          </div>
        </div>
        <Divider></Divider>
        <div className="flex w-full items-center justify-between">
          <div className="flex flex-col">
            <span>{t('compression_options.output_options.keep_structure')}</span>
          </div>
          <Switch isSelected={keepFolderStructure} size="sm" onValueChange={setKeepFolderStructure}></Switch>
        </div>

        <div className="flex w-full items-center justify-between">
          <div className="flex flex-col">
            <span>{t('compression_options.output_options.skip_if_output_is_bigger')}</span>
          </div>
          <Switch isSelected={skipIfOutputIsBigger} size="sm" onValueChange={setSkipIfOutputIsBigger}></Switch>
        </div>

        <div className="flex w-full flex-col justify-between gap-1">
          <div className="flex w-full items-center justify-between">
            <div className="flex flex-col">
              <span>{t('compression_options.output_options.move_original')}</span>
            </div>
            <Switch isSelected={moveOriginalFile} size="sm" onValueChange={setMoveOriginalFile}></Switch>
          </div>
          <Select
            disallowEmptySelection
            aria-label={t('compression_options.output_options.move_original')}
            classNames={{
              label: 'text-md',
              trigger: 'shadow-none',
            }}
            isDisabled={!moveOriginalFile}
            label={''}
            labelPlacement="outside"
            selectedKeys={[moveOriginalFileType]}
            selectionMode="single"
            size="sm"
            variant="faded"
            onSelectionChange={(value) =>
              setMoveOriginalFileType((value.currentKey as MOVE_ORIGINAL_FILE) || MOVE_ORIGINAL_FILE.TRASH)
            }
          >
            {moveOriginalFileTypes.map((t) => (
              <SelectItem key={t.key}>{t.label}</SelectItem>
            ))}
          </Select>
        </div>
        <Divider></Divider>
        <div className="flex flex-col gap-1">
          <Checkbox
            disableAnimation
            isIndeterminate={keepFileDates.length > 0 && keepFileDates.length < 3}
            isSelected={keepFileDates.length === 3}
            size="sm"
            onValueChange={(v) =>
              setKeepFileDates(v ? [FILE_DATE.CREATED, FILE_DATE.MODIFIED, FILE_DATE.ACCESSED] : [])
            }
          >
            {t('compression_options.output_options.keep_file_dates')}
          </Checkbox>

          <Checkbox
            disableAnimation
            className="ml-4"
            isSelected={keepFileDates.includes(FILE_DATE.CREATED)}
            size="sm"
            onValueChange={(v) => toggleFileDateCreation(FILE_DATE.CREATED, v)}
          >
            {t('file_dates.creation')}
          </Checkbox>
          <Checkbox
            disableAnimation
            className="ml-4"
            isSelected={keepFileDates.includes(FILE_DATE.MODIFIED)}
            size="sm"
            onValueChange={(v) => toggleFileDateCreation(FILE_DATE.MODIFIED, v)}
          >
            {t('file_dates.last_modified')}
          </Checkbox>
          <Checkbox
            disableAnimation
            className="ml-4"
            isSelected={keepFileDates.includes(FILE_DATE.ACCESSED)}
            size="sm"
            onValueChange={(v) => toggleFileDateCreation(FILE_DATE.ACCESSED, v)}
          >
            {t('file_dates.last_access')}
          </Checkbox>
        </div>
        <Divider></Divider>
        <Select
          disallowEmptySelection
          aria-label={t('compression_options.output_options.output_format')}
          classNames={{
            label: 'text-md',
            trigger: 'shadow-none',
          }}
          label={t('compression_options.output_options.output_format')}
          labelPlacement="outside"
          selectedKeys={[outputFormat]}
          selectionMode="single"
          size="sm"
          variant="faded"
          onSelectionChange={(value) => setOutputFormat((value.currentKey as OUTPUT_FORMAT) || OUTPUT_FORMAT.ORIGINAL)}
        >
          {outputFormats.map((t) => (
            <SelectItem key={t.key}>{t.label}</SelectItem>
          ))}
        </Select>

        <Input
          isClearable
          classNames={{
            inputWrapper: 'shadow-none',
          }}
          label={t('compression_options.output_options.suffix')}
          labelPlacement="outside"
          placeholder="_compressed"
          size="sm"
          value={suffix}
          variant="faded"
          onValueChange={(v) => setSuffix(v)}
        />
      </div>
    </div>
  );
}

export default OutputOptions;
