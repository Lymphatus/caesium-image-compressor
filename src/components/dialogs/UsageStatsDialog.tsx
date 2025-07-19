import { Button, Modal, ModalBody, ModalContent, ModalFooter, ModalHeader } from '@heroui/react';
import { useTranslation } from 'react-i18next';
import useCompressionOptionsStore from '@/stores/compression-options.store.ts';
import useResizeOptionsStore from '@/stores/resize-options.store.ts';
import useOutputOptionsStore, { OutputOptions } from '@/stores/output-options.store.ts';
import { app } from '@tauri-apps/api';
import { arch, platform, version } from '@tauri-apps/plugin-os';
import { Check, Copy, X } from 'lucide-react';
import { useState } from 'react';
import useAppStore from '@/stores/app.store.ts';
import { UsageStats } from '@/types.ts';

type UsageStatsDialogProps = {
  isOpen: boolean;
  onClose: () => void;
};

enum CopyStatus {
  PENDING,
  SUCCESS,
  ERROR,
}

const appVersion = await app.getVersion();

const copyIcon = <Copy className="size-4" />;
const copySuccessIcon = <Check className="text-success size-4" />;
const copyErrorIcon = <X className="text-danger size-4" />;

function UsageStatsDialog({ isOpen, onClose }: UsageStatsDialogProps) {
  const { t } = useTranslation();
  const { uuid } = useAppStore();
  const [copyStatus, setCopyStatus] = useState(CopyStatus.PENDING);

  const outputOptions = { ...useOutputOptionsStore.getState().getOutputOptions() } as Partial<OutputOptions>;
  delete outputOptions.output_folder;

  const usageStats: UsageStats = {
    system: {
      uuid: uuid,
      appVersion,
      cpuArchitecture: arch(),
      productType: platform(),
      productVersion: version(),
    },
    compression: {
      compressionOptions: useCompressionOptionsStore.getState().getCompressionOptions(),
      resizeOptions: useResizeOptionsStore.getState().getResizeOptions(),
      outputOptions: outputOptions as Omit<OutputOptions, 'output_folder'>,
    },
  };

  const formattedUsageStats = JSON.stringify(usageStats, null, 2);

  const onCopyPressed = () => {
    navigator.clipboard
      .writeText(formattedUsageStats)
      .then(() => setCopyStatus(CopyStatus.SUCCESS))
      .catch(() => setCopyStatus(CopyStatus.ERROR))
      .finally(() => {
        setTimeout(() => {
          setCopyStatus(CopyStatus.PENDING);
        }, 2000);
      });
  };

  const copyStatusIcon = () => {
    switch (copyStatus) {
      case CopyStatus.PENDING:
        return copyIcon;
      case CopyStatus.SUCCESS:
        return copySuccessIcon;
      case CopyStatus.ERROR:
        return copyErrorIcon;
    }
  };
  return (
    <Modal
      isDismissable
      backdrop="blur"
      classNames={{
        backdrop: 'bg-content3/50',
      }}
      isOpen={isOpen}
      shadow="none"
      size="xl"
      onClose={onClose}
    >
      <ModalContent>
        <ModalHeader className="flex flex-col gap-1">{t('usage_stats')}</ModalHeader>
        <ModalBody>
          <div className="bg-default/40 text-default-700 text-small inline-block h-fit max-h-40 w-full overflow-auto rounded-none px-2 py-1 font-mono font-normal whitespace-nowrap">
            <pre className="">{formattedUsageStats}</pre>
          </div>
        </ModalBody>
        <ModalFooter>
          <div className="flex w-full justify-between gap-2">
            <Button disableRipple radius="sm" startContent={copyStatusIcon()} variant="light" onPress={onCopyPressed}>
              {t('copy')}
            </Button>
            <Button disableRipple radius="sm" onPress={onClose}>
              {t('close')}
            </Button>
          </div>
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
}

export default UsageStatsDialog;
