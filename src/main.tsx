import React from 'react';
import ReactDOM from 'react-dom/client';
import App from './App';
import { HeroUIProvider, ToastProvider } from '@heroui/react';
import './assets/css/App.css';
import './i18n';
import { ErrorBoundary } from '@/components/ErrorBoundary.tsx';

ReactDOM.createRoot(document.getElementById('root') as HTMLElement).render(
  <React.StrictMode>
    <ErrorBoundary>
      <HeroUIProvider>
        <ToastProvider />
        <main className="dark:bg-background bg-default-200 text-foreground h-screen w-screen text-center">
          <App />
        </main>
      </HeroUIProvider>
    </ErrorBoundary>
  </React.StrictMode>,
);
