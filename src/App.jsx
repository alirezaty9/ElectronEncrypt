import React from 'react'
import { RouterProvider } from 'react-router-dom'
import routes from '../routes'
import TokenGuard from './components/TokenGuard'

export default function App() {
  return (
    <TokenGuard>
      <RouterProvider router={routes}/>
    </TokenGuard>
  )
}