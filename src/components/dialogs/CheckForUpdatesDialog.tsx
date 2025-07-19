import { Button, Modal, ModalBody, ModalContent, ModalFooter, ModalHeader, Progress } from '@heroui/react';
import useUIStore from '@/stores/ui.store.ts';
import { check, DownloadEvent, Update } from '@tauri-apps/plugin-updater';
import { useEffect, useState } from 'react';
import { useTranslation } from 'react-i18next';
import { CircleAlert, CircleCheck, TriangleAlert } from 'lucide-react';
import { relaunch } from '@tauri-apps/plugin-process';

enum UpdateStatus {
  CHECKING,
  AVAILABLE,
  NOT_AVAILABLE,
  DOWNLOADING,
  READY_TO_INSTALL,
  ERROR,
}

function CheckForUpdatesDialog() {
  const { setCheckForUpdatesDialogOpen, checkForUpdatesDialogOpen } = useUIStore();
  const { t } = useTranslation();

  const [update, setUpdate] = useState<Update | null>(null);
  const [updateStatus, setUpdateStatus] = useState<UpdateStatus>(UpdateStatus.CHECKING);
  const [error, setError] = useState<string | null>(null);
  const [bytesDownloaded, setBytesDownloaded] = useState(0);
  const [bytesTotal, setBytesTotal] = useState(0);

  useEffect(() => {
    if (!checkForUpdatesDialogOpen) {
      return;
    }
    check({
      timeout: 5000,
    })
      .then((u) => {
        setUpdate(u);
        if (u === null) {
          setUpdateStatus(UpdateStatus.NOT_AVAILABLE);
        } else {
          setUpdateStatus(UpdateStatus.AVAILABLE);
        }
      })
      .catch((e) => {
        setError(e);
        setUpdateStatus(UpdateStatus.ERROR);
      });

    return () => {
      setUpdateStatus(UpdateStatus.CHECKING);
      setUpdate(null);
      setError(null);
      setBytesDownloaded(0);
      setBytesTotal(0);
    };
  }, [checkForUpdatesDialogOpen]);

  const downloadCallback = (event: DownloadEvent) => {
    switch (event.event) {
      case 'Started':
        setBytesTotal(event.data.contentLength || 0);
        break;
      case 'Progress':
        setBytesDownloaded((prev) => prev + event.data.chunkLength);
        break;
      case 'Finished':
        setUpdateStatus(UpdateStatus.READY_TO_INSTALL);
        break;
    }
  };
  const startDownload = async () => {
    if (!update) {
      return;
    }
    setUpdateStatus(UpdateStatus.DOWNLOADING);
    setBytesDownloaded(0);
    setBytesTotal(0);

    await update.download(downloadCallback);
  };

  const installUpdate = async () => {
    if (!update) {
      return;
    }
    await update.install();
    await relaunch();
  };

  let message = <span>{t('update_process.checking_for_updates_dots')}</span>;
  let updateButton = (
    <Button disableRipple color="primary" isDisabled={update === null} onPress={startDownload}>
      {t('update')}
    </Button>
  );
  if (updateStatus === UpdateStatus.NOT_AVAILABLE) {
    message = (
      <span className="flex items-center gap-1">
        <CircleCheck className="text-success size-4"></CircleCheck>
        {t('update_process.latest_version')}
      </span>
    );
  } else if (updateStatus === UpdateStatus.AVAILABLE && update) {
    message = (
      <span className="flex items-center gap-1">
        <TriangleAlert className="text-warning size-4"></TriangleAlert> {t('update_process.new_update_available')}:
        <span className="font-bold">v{update.version}</span>
      </span>
    );
  } else if (updateStatus === UpdateStatus.ERROR) {
    message = (
      <span className="flex items-center gap-1">
        <CircleAlert className="text-danger size-4"></CircleAlert>
        {t('errors.generic_retry')}
      </span>
    );
  } else if (updateStatus === UpdateStatus.DOWNLOADING) {
    message = <span>{t('downloading_dots')}</span>;
  } else if (updateStatus === UpdateStatus.READY_TO_INSTALL) {
    message = (
      <span className="flex items-center gap-1">
        <CircleCheck className="text-success size-4"></CircleCheck>
        {t('update_process.ready_to_install')}
      </span>
    );

    updateButton = (
      <Button disableRipple color="primary" onPress={installUpdate}>
        {t('install')}
      </Button>
    );
  }

  return (
    <Modal
      isDismissable
      backdrop="blur"
      classNames={{
        backdrop: 'bg-content3/50',
      }}
      isOpen={checkForUpdatesDialogOpen}
      shadow="none"
      size="sm"
      onClose={() => setCheckForUpdatesDialogOpen(false)}
    >
      <ModalContent>
        <ModalHeader className="flex flex-col gap-1"></ModalHeader>
        <ModalBody>
          <div className="flex flex-col gap-4">
            <Progress
              disableAnimation
              isIndeterminate={updateStatus === UpdateStatus.CHECKING}
              label={message}
              maxValue={bytesTotal}
              minValue={0}
              showValueLabel={updateStatus === UpdateStatus.DOWNLOADING}
              size="sm"
              value={bytesDownloaded}
              valueLabel={<span>{Math.round((bytesDownloaded / bytesTotal) * 100) || 0}%</span>}
            ></Progress>
            <span className="text-danger text-sm" hidden={!error}>
              {error}
            </span>
            {update && update.body && (
              <div className="flex flex-col gap-2">
                <span className="text-sm">{t('changelog')}</span>
                <div className="bg-default/40 text-default-700 text-small inline-block max-h-40 min-h-20 w-full overflow-auto rounded-sm px-2 py-1 font-mono font-normal whitespace-nowrap">
                  <pre className="text-sm">{update?.body}</pre>
                </div>
              </div>
            )}
          </div>
        </ModalBody>
        <ModalFooter>
          <div className="flex w-full items-center justify-between gap-2">
            <div>
              <Button disableRipple variant="flat" onPress={() => setCheckForUpdatesDialogOpen(false)}>
                {t('cancel')}
              </Button>
            </div>
            <div className="flex gap-2">{updateButton}</div>
          </div>
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
}

export default CheckForUpdatesDialog;
