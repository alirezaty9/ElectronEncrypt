import { createHashRouter } from 'react-router-dom';
import Layout from './src/components/Layout';
import Home from './src/pages/Home';
import EncryptionPage from './src/pages/EncryptionPage';
import DecryptionPage from './src/pages/DecryptionPage';

const router = createHashRouter([
  {
    path: '/',
    element: <Layout />,
    children: [
      {
        index: true,
        element: <Home />
      },
      {
        path: 'encrypt',
        element: <EncryptionPage />
      },
      {
        path: 'decrypt',
        element: <DecryptionPage />
      }
    ]
  }
]);

export default router;