import { useTranslation } from 'react-i18next';
import useSettingsStore from '@/stores/settings.store.ts';
import { DIRECT_IMPORT_ACTION, POST_COMPRESSION_ACTION } from '@/types.ts';
import { Select, SelectItem, Slider } from '@heroui/react';

function AdvancedSettings() {
  const { t } = useTranslation();
  const {
    directImportAction,
    postCompressionAction,
    threadsCount,
    threadsPriority,
    maxThreads,
    setDirectImportAction,
    setPostCompressionAction,
    setThreadsCount,
    setThreadsPriority,
  } = useSettingsStore();

  const directImportActions = [
    { key: DIRECT_IMPORT_ACTION.IMPORT, label: t('settings.direct_import_actions.import_only') },
    { key: DIRECT_IMPORT_ACTION.IMPORT_COMPRESS, label: t('settings.direct_import_actions.import_and_compress') },
  ];

  const postCompressionActions = [
    { key: POST_COMPRESSION_ACTION.NONE, label: t('settings.post_compression_actions.none') },
    { key: POST_COMPRESSION_ACTION.CLOSE_APP, label: t('settings.post_compression_actions.close_application') },
    { key: POST_COMPRESSION_ACTION.SLEEP, label: t('settings.post_compression_actions.sleep') },
    { key: POST_COMPRESSION_ACTION.SHUTDOWN, label: t('settings.post_compression_actions.shutdown') },
    {
      key: POST_COMPRESSION_ACTION.OPEN_OUTPUT_FOLDER,
      label: t('settings.post_compression_actions.open_output_folder'),
    },
  ];

  const threadsValue = Math.min(threadsCount, maxThreads);
  return (
    <div className="h-full">
      <div className="flex size-full flex-col gap-4">
        <Select
          disallowEmptySelection
          aria-label={t('settings.direct_import_action')}
          classNames={{
            base: 'justify-between',
            mainWrapper: 'max-w-[250px]',
            label: 'text-md',
            trigger: 'shadow-none',
          }}
          label={
            <div className="flex flex-col">
              <span>{t('settings.direct_import_action')}</span>
              <span className="text-default-500 text-sm">{t('settings.direct_import_action_help')}</span>
            </div>
          }
          labelPlacement="outside-left"
          selectedKeys={[directImportAction]}
          selectionMode="single"
          size="sm"
          variant="faded"
          onSelectionChange={(value) =>
            setDirectImportAction((value.currentKey as DIRECT_IMPORT_ACTION) || DIRECT_IMPORT_ACTION.IMPORT)
          }
        >
          {directImportActions.map((t) => (
            <SelectItem key={t.key}>{t.label}</SelectItem>
          ))}
        </Select>
        <Select
          disallowEmptySelection
          aria-label={t('settings.post_compression_action')}
          classNames={{
            base: 'justify-between',
            mainWrapper: 'max-w-[250px]',
            label: 'text-md',
            trigger: 'shadow-none',
          }}
          label={
            <div className="flex flex-col">
              <span>{t('settings.post_compression_action')}</span>
              <span className="text-default-500 text-sm">{t('settings.post_compression_action_help')}</span>
            </div>
          }
          labelPlacement="outside-left"
          selectedKeys={[postCompressionAction]}
          selectionMode="single"
          size="sm"
          variant="faded"
          onSelectionChange={(value) =>
            setPostCompressionAction((value.currentKey as POST_COMPRESSION_ACTION) || POST_COMPRESSION_ACTION.NONE)
          }
        >
          {postCompressionActions.map((t) => (
            <SelectItem key={t.key}>{t.label}</SelectItem>
          ))}
        </Select>
        <div className="flex w-full items-center justify-between">
          <div className="flex flex-col">
            <span>{t('settings.max_compression_threads')}</span>
            {/*<span className="text-default-500 text-sm">{t('settings.send_usage_statistics_help')}</span>*/}
          </div>
          <Slider
            aria-label={t('settings.max_compression_threads')}
            className="max-w-[250px]"
            classNames={{
              label: 'text-sm',
            }}
            endContent={<span>{threadsValue}</span>}
            maxValue={maxThreads}
            minValue={1}
            size="sm"
            step={1}
            value={threadsValue}
            onChange={(v) => {
              if (Array.isArray(v)) {
                v = v[0];
              }

              setThreadsCount(v);
            }}
          />
        </div>
        <div className="flex w-full items-center justify-between">
          <div className="flex flex-col">
            <span>{t('settings.threads_priority')}</span>
            {/*<span className="text-default-500 text-sm">{t('settings.send_usage_statistics_help')}</span>*/}
          </div>
          <Slider
            hideValue
            showSteps
            aria-label={t('settings.threads_priority')}
            className="max-w-[250px]"
            classNames={{
              label: 'text-sm',
            }}
            marks={[
              { value: 1, label: t('settings.threads_priorities.lowest') },
              { value: 4, label: t('settings.threads_priorities.normal') },
              { value: 7, label: t('settings.threads_priorities.highest') },
            ]}
            maxValue={7}
            minValue={1}
            size="sm"
            step={1}
            value={threadsPriority}
            onChange={(v) => {
              if (Array.isArray(v)) {
                v = v[0];
              }

              setThreadsPriority(v);
            }}
          />
        </div>
      </div>
    </div>
  );
}

export default AdvancedSettings;
