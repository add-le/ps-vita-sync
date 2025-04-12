import { exec } from "node:child_process";
import packageJson from "../package.json";

const config_file: string = "src/appconfig.h";
const cmake_file: string = "CMakeLists.txt";
const app_version: string = packageJson.version;

exec(
  `sed -i -e 's/APP_VERSION [[:alnum:]."]*/APP_VERSION "${app_version}"/g' ${config_file} &&
   sed -i -e 's/VITA_VERSION [[:alnum:]."]*/VITA_VERSION "${app_version}"/g' ${cmake_file}`
);
