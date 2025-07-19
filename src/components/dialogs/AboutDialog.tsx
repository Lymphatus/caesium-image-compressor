import { Modal, ModalBody, ModalContent, ModalFooter, ModalHeader } from '@heroui/react';
import { useTranslation } from 'react-i18next';
import useUIStore from '@/stores/ui.store';
import appLogo from '@/assets/images/app-icon.png';
import { app } from '@tauri-apps/api';
import useAppStore from '@/stores/app.store.ts';

const appVersion = await app.getVersion();

function AboutDialog() {
  const { aboutDialogOpen, setAboutDialogOpen } = useUIStore();
  const { uuid } = useAppStore();
  const { t } = useTranslation();

  return (
    <Modal
      backdrop="blur"
      classNames={{
        backdrop: 'bg-content3/50',
      }}
      isOpen={aboutDialogOpen}
      shadow="none"
      size="sm"
      onClose={() => setAboutDialogOpen(false)}
    >
      <ModalContent>
        <ModalHeader className="flex flex-col gap-1"></ModalHeader>
        <ModalBody>
          <div className="flex flex-col items-center justify-center gap-4 text-center">
            <img alt="Caesium Logo" className="mx-auto mb-4 w-32" src={appLogo} />
            <div className="flex flex-col items-center justify-center">
              <div className="text-xl font-bold">{t('app_name')}</div>
              <span className="text-sm">v{appVersion}</span>
            </div>

            <div className="flex flex-col items-center justify-center">
              <small className="font-mono text-xs">UUID: {uuid}</small>
              <a
                className="text-primary text-sm hover:underline"
                href="https://saerasoft.com/caesium"
                rel="noreferrer"
                target="_blank"
              >
                saerasoft.com/caesium
              </a>
            </div>
          </div>
        </ModalBody>

        <ModalFooter>
          <div className="w-full text-center">
            <small className="text-xs">{t('copyright_note')}</small>
          </div>
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
}

export default AboutDialog;
