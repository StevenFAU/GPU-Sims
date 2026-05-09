// Lightweight console wrapper. Mirrors common-cpp's log.hpp shape.

export type LogLevel = 'debug' | 'info' | 'warn' | 'error';

let initialized = false;
let minLevel: LogLevel = 'info';

const levelOrder: Record<LogLevel, number> = { debug: 0, info: 1, warn: 2, error: 3 };

function timestamp(): string {
    const d = new Date();
    const hh = String(d.getHours()).padStart(2, '0');
    const mm = String(d.getMinutes()).padStart(2, '0');
    const ss = String(d.getSeconds()).padStart(2, '0');
    const ms = String(d.getMilliseconds()).padStart(3, '0');
    return `${hh}:${mm}:${ss}.${ms}`;
}

function shouldLog(level: LogLevel): boolean {
    return levelOrder[level] >= levelOrder[minLevel];
}

export interface LoggerOptions {
    level?: LogLevel;
}

export function initLogger(options: LoggerOptions = {}): void {
    if (initialized) return;
    initialized = true;
    minLevel = options.level ?? (import.meta.env?.DEV ? 'debug' : 'info');
    log.info('logger initialized');
}

export const log = {
    debug(msg: string, ...rest: unknown[]): void {
        if (!shouldLog('debug')) return;
        // eslint-disable-next-line no-console
        console.debug(`[${timestamp()}] [debug]`, msg, ...rest);
    },
    info(msg: string, ...rest: unknown[]): void {
        if (!shouldLog('info')) return;
        // eslint-disable-next-line no-console
        console.info(`[${timestamp()}] [info]`, msg, ...rest);
    },
    warn(msg: string, ...rest: unknown[]): void {
        if (!shouldLog('warn')) return;
        // eslint-disable-next-line no-console
        console.warn(`[${timestamp()}] [warn]`, msg, ...rest);
    },
    error(msg: string, ...rest: unknown[]): void {
        if (!shouldLog('error')) return;
        // eslint-disable-next-line no-console
        console.error(`[${timestamp()}] [error]`, msg, ...rest);
    },
};
