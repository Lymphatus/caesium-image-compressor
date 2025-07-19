import { useEffect } from 'react';
import Toolbar from '@/components/Toolbar.tsx';
import Footer from '@/components/Footer.tsx';
import ImportDialog from '@/components/dialogs/ImportDialog.tsx';
import CenterContainer from '@/components/CenterContainer.tsx';
import useFileListStore from '@/stores/file-list.store.ts';
import { listen, TauriEvent } from '@tauri-apps/api/event';
import { CImage } from '@/types.ts';
import { addToast, Button } from '@heroui/react';
import SettingsDialog from '@/components/dialogs/SettingsDialog.tsx';
import usePreviewStore from '@/stores/preview.store.ts';
import AboutDialog from './components/dialogs/AboutDialog';
import { invoke } from '@tauri-apps/api/core';
import { getCurrentWindow } from '@tauri-apps/api/window';
import useSettingsStore from '@/stores/settings.store.ts';
import useUIStore from '@/stores/ui.store.ts';
import AskDialog from '@/components/dialogs/AskDialog.tsx';
import { useTranslation } from 'react-i18next';
import CheckForUpdatesDialog from '@/components/dialogs/CheckForUpdatesDialog.tsx';

function App() {
  const {
    setFileList,
    setBaseFolder,
    setIsImporting,
    setTotalFiles,
    setImportProgress,
    updateFile,
    setCompressionProgress,
    currentPage,
  } = useFileListStore();

  const { getCurrentPreviewedCImage } = usePreviewStore();
  const { promptExitDialogOpen, setPromptExitDialogOpen } = useUIStore();
  const { skipMessagesAndDialogs } = useSettingsStore();
  const { t } = useTranslation();

  useEffect(() => {
    const importFinishedListener = listen('fileImporter:importFinished', () => {
      setIsImporting(false);
      //TODO correct text and translation
      addToast({
        title: 'Import finished',
        description: 'Imported a few files',
        // timeout: 3000,
        color: 'success',
      });
    });

    const getListListener = listen<{ files: CImage[]; base_folder: string; total_files: number }>(
      'fileList:getList',
      (event) => {
        console.log(event);
        const { files, base_folder, total_files } = event.payload;
        setFileList(files);
        setBaseFolder(base_folder);
        setTotalFiles(total_files);
        setIsImporting(false);
      },
    );

    const importStartedListener = listen('fileImporter:importStarted', () => {
      setImportProgress(0);
      setIsImporting(true);
    });

    const importProgressListener = listen<{ progress: number; total: number }>(
      'fileImporter:importProgress',
      (event) => {
        const { progress } = event.payload;
        setImportProgress(progress);
      },
    );

    const updateCImageListener = listen<{ status: number; cimage: CImage }>('fileList:updateCImage', async (event) => {
      const { cimage } = event.payload;
      updateFile(cimage.id, cimage);
      if (getCurrentPreviewedCImage()?.id === cimage.id) {
        usePreviewStore.setState({ currentPreviewedCImage: cimage });
      }
    });

    const updateCompressionProgressListener = listen<number>('fileList:compressionProgress', async (event) => {
      setCompressionProgress(event.payload);
    });

    const closeRequestedListener = getCurrentWindow().listen(TauriEvent.WINDOW_CLOSE_REQUESTED, async () => {
      if (useSettingsStore.getState().promptBeforeExit && !useSettingsStore.getState().skipMessagesAndDialogs) {
        setPromptExitDialogOpen(true);
      } else {
        //Avoid infinite loop
        await getCurrentWindow().destroy();
      }
    });

    invoke('change_page', { page: currentPage }).then();

    return () => {
      Promise.all([
        importFinishedListener,
        getListListener,
        importStartedListener,
        importProgressListener,
        updateCImageListener,
        closeRequestedListener,
        updateCompressionProgressListener,
      ]).then((cleanupFns) => {
        cleanupFns.forEach((cleanupFn) => cleanupFn());
      });
    };
  }, []);

  return (
    <>
      <Toolbar></Toolbar>
      <CenterContainer></CenterContainer>
      <Footer></Footer>

      <ImportDialog></ImportDialog>
      <SettingsDialog></SettingsDialog>
      <AboutDialog></AboutDialog>
      <CheckForUpdatesDialog></CheckForUpdatesDialog>
      <AskDialog
        buttons={
          <>
            <Button disableRipple color="primary" onPress={async () => await getCurrentWindow().destroy()}>
              {t('affirmative_answer')}
            </Button>
            <Button disableRipple onPress={() => setPromptExitDialogOpen(false)}>
              {t('negative_answer')}
            </Button>
          </>
        }
        isOpen={promptExitDialogOpen && !skipMessagesAndDialogs}
        message={t('confirm_exit_message')}
      ></AskDialog>
    </>
  );
}

export default App;
