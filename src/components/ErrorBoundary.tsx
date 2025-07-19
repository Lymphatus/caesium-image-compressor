import * as React from 'react';
import { error } from '@tauri-apps/plugin-log';

interface ErrorBoundaryProps {
  children: React.ReactNode;
}

interface ErrorBoundaryState {
  hasError: boolean;
}

export class ErrorBoundary extends React.Component<ErrorBoundaryProps, ErrorBoundaryState> {
  constructor(props: ErrorBoundaryProps) {
    super(props);
    this.state = { hasError: false };
  }

  static getDerivedStateFromError(): ErrorBoundaryState {
    return { hasError: true };
  }

  componentDidCatch(e: Error, info: React.ErrorInfo): void {
    void error([e, info.componentStack, React.captureOwnerStack()].join('\n'));
  }

  render(): React.ReactNode {
    if (this.state.hasError) {
      return <div className="flex h-screen w-screen items-center justify-center">An error has occurred :(</div>;
    }

    return this.props.children;
  }
}
