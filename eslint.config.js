export default [
  {
    ignores: ['dist/**', 'node_modules/**']
  },
  {
    files: ['**/*.{js,jsx}'],
    rules: {
      // فقط خطاهای واقعی
      'no-undef': 'off',
      'no-unused-vars': 'off', 
      'react-hooks/exhaustive-deps': 'off',
      'react-refresh/only-export-components': 'off'
    }
  }
];