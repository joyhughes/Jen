import globals from "globals";
import pluginJs from "@eslint/js";
import pluginReactConfig from "eslint-plugin-react/configs/recommended.js";

export default [
  {languageOptions: { globals: globals.browser }},
  pluginJs.configs.recommended,
  pluginReactConfig,
    {
        name: "eslint/global-rules",
        rules: {
            "no-octal-escape": "off",
	    }
    },
];
