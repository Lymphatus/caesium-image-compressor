import { Modal, ModalBody, ModalContent, ModalFooter, ModalHeader, Progress } from '@heroui/react';
import useFileListStore from '@/stores/file-list.store.ts';
import { useTranslation } from 'react-i18next';

function ImportDialog() {
  const { isImporting, importProgress } = useFileListStore();
  const { t } = useTranslation();

  return (
    <Modal
      hideCloseButton
      isKeyboardDismissDisabled
      backdrop="blur"
      classNames={{
        backdrop: 'bg-content3/50',
      }}
      isDismissable={false}
      isOpen={isImporting}
      shadow="none"
      size="sm"
    >
      <ModalContent>
        <ModalHeader className="flex flex-col gap-1">{t('importing_dots')}</ModalHeader>
        <ModalBody>
          <Progress
            disableAnimation
            aria-label={t('importing_dots')}
            className="w-full"
            isIndeterminate={!importProgress}
            size="sm"
            value={importProgress || 0}
          ></Progress>
        </ModalBody>
        <ModalFooter></ModalFooter>
      </ModalContent>
    </Modal>
  );
}

export default ImportDialog;
