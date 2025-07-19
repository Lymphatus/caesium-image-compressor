import i18n from 'i18next';
import { initReactI18next } from 'react-i18next';

import en_US from '@/i18n/en-US.json';
import it_IT from '@/i18n/it-IT.json';

const resources = {
  'en-US': {
    translation: en_US,
  },
  'it-IT': {
    translation: it_IT,
  },
};

i18n.use(initReactI18next).init({
  resources,
  lng: 'en-US',
  fallbackLng: 'en-US',
  interpolation: {
    escapeValue: false,
  },
});

export default i18n;
