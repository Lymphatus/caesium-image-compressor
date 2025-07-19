import { Button, Modal, ModalBody, ModalContent, ModalFooter, ModalHeader, Tab, Tabs } from '@heroui/react';
import useUIStore from '@/stores/ui.store.ts';
import { useTranslation } from 'react-i18next';
import { Code, Import, Settings2 } from 'lucide-react';
import GeneralSettings from '@/components/dialogs/settings/GeneralSettings.tsx';
import ImportSettings from '@/components/dialogs/settings/ImportSettings.tsx';
import AdvancedSettings from '@/components/dialogs/settings/AdvancedSettings.tsx';

function SettingsDialog() {
  const { setSettingsDialogOpen, settingsDialogOpen } = useUIStore();
  const { t } = useTranslation();

  return (
    <Modal
      backdrop="blur"
      className="min-h-[50%] rounded-sm shadow-none"
      classNames={{
        backdrop: 'bg-content3/50',
      }}
      isOpen={settingsDialogOpen}
      shadow="none"
      size="3xl"
      onClose={() => setSettingsDialogOpen(false)}
      onOpenChange={() => {}}
    >
      <ModalContent>
        <ModalHeader className="flex flex-col gap-1">{t('settings_title')}</ModalHeader>
        <ModalBody>
          <Tabs
            isVertical
            aria-label={t('settings_title')}
            classNames={{
              tabWrapper: 'flex-1 size-full',
              tabList: 'rounded-sm',
              tab: 'justify-start h-full',
              panel: 'pr-0 w-full',
            }}
          >
            <Tab
              key="general"
              title={
                <div className="flex items-center space-x-2">
                  <Settings2 />
                  <span>{t('settings.general')}</span>
                </div>
              }
            >
              <GeneralSettings></GeneralSettings>
            </Tab>
            <Tab
              key="import"
              title={
                <div className="flex items-center space-x-2">
                  <Import />
                  <span>{t('settings.import')}</span>
                </div>
              }
            >
              <ImportSettings></ImportSettings>
            </Tab>
            <Tab
              key="advanced"
              title={
                <div className="flex items-center space-x-2">
                  <Code />
                  <span>{t('settings.advanced')}</span>
                </div>
              }
            >
              <AdvancedSettings></AdvancedSettings>
            </Tab>
          </Tabs>
        </ModalBody>
        <ModalFooter>
          <Button disableRipple onPress={() => setSettingsDialogOpen(false)}>
            {t('close')}
          </Button>
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
}

export default SettingsDialog;
