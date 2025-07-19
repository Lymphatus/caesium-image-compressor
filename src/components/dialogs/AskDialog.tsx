import { Modal, ModalBody, ModalContent, ModalFooter, ModalHeader } from '@heroui/react';
import useUIStore from '@/stores/ui.store.ts';
import { ReactNode } from 'react';

type AskDialogProps = {
  title?: string;
  message?: string;
  buttons?: ReactNode;
  isOpen: boolean;
};

function AskDialog({ title, message, buttons, isOpen }: AskDialogProps) {
  const { setPromptExitDialogOpen } = useUIStore();
  return (
    <Modal
      isDismissable
      backdrop="blur"
      classNames={{
        backdrop: 'bg-content3/50',
      }}
      isOpen={isOpen}
      shadow="none"
      size="sm"
      onClose={() => setPromptExitDialogOpen(false)}
    >
      <ModalContent>
        <ModalHeader className="flex flex-col gap-1">{title}</ModalHeader>
        <ModalBody>{message}</ModalBody>
        <ModalFooter>{buttons}</ModalFooter>
      </ModalContent>
    </Modal>
  );
}

export default AskDialog;
