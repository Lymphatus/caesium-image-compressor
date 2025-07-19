import useFileListStore from '@/stores/file-list.store.ts';
import { Button, Progress } from '@heroui/react';
import { useTranslation } from 'react-i18next';
import i18next from 'i18next';
import { X } from 'lucide-react';
import { warn } from '@tauri-apps/plugin-log';

function Footer() {
  const { baseFolder, totalFiles, compressionProgress, isCompressing } = useFileListStore();
  const { t } = useTranslation();

  return (
    <div className="bg-content1 flex h-[30px] w-full items-center justify-between px-4 text-sm">
      <div>
        <div>
          {totalFiles} | {baseFolder}
        </div>
      </div>
      {isCompressing && (
        <div className="flex items-center justify-center gap-2">
          <span className="text-xs">
            {t('compressing_dots')} [{compressionProgress}/{totalFiles}]
          </span>
          <div className="w-60">
            <Progress
              disableAnimation
              aria-label="compressionProgress"
              className="w-full"
              maxValue={totalFiles}
              minValue={0}
              size="sm"
              value={compressionProgress}
            ></Progress>
          </div>
          <Button
            disableRipple
            isIconOnly
            className="max-h-[30px]"
            size="sm"
            title={i18next.t('cancel')}
            variant="light"
            onPress={async () => {
              await warn('Cancel button pressed, but functionality not implemented yet.');
            }}
          >
            <X className="size-3"></X>
          </Button>
        </div>
      )}
    </div>
  );
}

export default Footer;
