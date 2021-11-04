import { handleResponse } from "./handle-response";
import { authHeader } from './auth-header';

export default function authFetch(url, options) {
  return fetch(url, { ...options, headers: { ...authHeader(), ...options.headers } })
    .then(handleResponse);
}
