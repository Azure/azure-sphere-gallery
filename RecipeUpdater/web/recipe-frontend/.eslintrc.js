module.exports = {
  env: {
    browser: true,
    es2021: true
  },
  extends: ["plugin:react/recommended", "airbnb"],
  parserOptions: {
    ecmaFeatures: {
      jsx: true
    },
    ecmaVersion: 12,
    sourceType: "module"
  },
  plugins: ["react"],
  rules: {
    "react/jsx-filename-extension": [1, { extensions: [".js", ".jsx"] }],
    "linebreak-style": 0,
    "import/prefer-default-export": 0,
    "react/jsx-props-no-spreading": 0,
    "react/prop-types": 0,
    "comma-dangle": ["error", "never"],
    "import/no-unresolved": 0,
    "import/no-extraneous-dependencies": ["error", { devDependencies: true }],
    "quote-props": 0,
    "jsx-a11y/media-has-caption": 0,
    "max-len": 0,
    "quotes": 0
  }
};
