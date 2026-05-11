/**
 * SML Logger - Conditional Logging System
 *
 * Development logging with DEBUG flag.
 * In production, set DEBUG=false to disable all logs.
 */

// Set to false in production
const DEBUG = true;

// Logging functions that only work when DEBUG=true
const log = {
  debug: DEBUG ? console.log : () => {},
  info: DEBUG ? console.log : () => {},
  warn: console.warn, // Always show warnings
  error: console.error, // Always show errors
};

// Namespace-specific loggers
const createLogger = (namespace) => ({
  debug: DEBUG ? (...args) => console.log(`[${namespace}]`, ...args) : () => {},
  info: DEBUG ? (...args) => console.log(`[${namespace}]`, ...args) : () => {},
  warn: (...args) => console.warn(`[${namespace}]`, ...args),
  error: (...args) => console.error(`[${namespace}]`, ...args),
});

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
  module.exports = { log, createLogger, DEBUG };
}
