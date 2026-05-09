import GUI from 'lil-gui';
import type { Controller } from 'lil-gui';

export interface ParamFolder {
    add<T extends object>(target: T, prop: keyof T & string, ...args: unknown[]): Controller;
    addFolder(name: string): ParamFolder;
    addBoolean<T extends object>(target: T, prop: keyof T & string): Controller;
    addNumber<T extends object>(target: T, prop: keyof T & string,
                                min?: number, max?: number, step?: number): Controller;
    addColor<T extends object>(target: T, prop: keyof T & string): Controller;
    addButton(name: string, fn: () => void): Controller;
    open(): void;
    close(): void;
}

class FolderImpl implements ParamFolder {
    constructor(private gui: GUI, private storageKey: string | null) {}

    add<T extends object>(target: T, prop: keyof T & string, ...args: unknown[]): Controller {
        const c = (this.gui.add as (...a: unknown[]) => Controller)(target, prop, ...args);
        if (this.storageKey) this.attachPersistence(c, target, prop);
        return c;
    }

    addNumber<T extends object>(target: T, prop: keyof T & string,
                                min?: number, max?: number, step?: number): Controller {
        const c = this.gui.add(target, prop, min as number, max as number, step as number);
        if (this.storageKey) this.attachPersistence(c, target, prop);
        return c;
    }

    addBoolean<T extends object>(target: T, prop: keyof T & string): Controller {
        const c = this.gui.add(target, prop);
        if (this.storageKey) this.attachPersistence(c, target, prop);
        return c;
    }

    addColor<T extends object>(target: T, prop: keyof T & string): Controller {
        const c = this.gui.addColor(target, prop);
        if (this.storageKey) this.attachPersistence(c, target, prop);
        return c;
    }

    addButton(name: string, fn: () => void): Controller {
        const obj = { [name]: fn };
        return this.gui.add(obj, name);
    }

    addFolder(name: string): ParamFolder {
        const folder = this.gui.addFolder(name);
        return new FolderImpl(folder, this.storageKey);
    }

    open(): void { this.gui.open(); }
    close(): void { this.gui.close(); }

    private attachPersistence<T extends object>(c: Controller, target: T,
                                                prop: keyof T & string): void {
        if (!this.storageKey) return;
        const fullKey = `${this.storageKey}:${prop}`;
        const stored = localStorage.getItem(fullKey);
        if (stored !== null) {
            try {
                const parsed: unknown = JSON.parse(stored);
                (target as Record<string, unknown>)[prop] = parsed;
                c.updateDisplay();
            } catch {
                // Ignore corrupted entry.
            }
        }
        c.onFinishChange((v: unknown) => {
            try { localStorage.setItem(fullKey, JSON.stringify(v)); } catch { /* full storage */ }
        });
    }
}

/**
 * Top-level parameter panel. Wrap sim parameters in plain objects, then bind:
 *
 *     const params = { speed: 1.0, color: '#ff0040' };
 *     const panel = new ParamPanel({ title: 'Sim', persistKey: 'strange-attractors' });
 *     panel.addNumber(params, 'speed', 0, 10);
 *     panel.addColor(params, 'color');
 */
export class ParamPanel implements ParamFolder {
    private gui: GUI;
    private folder: FolderImpl;

    constructor(options: {
        title?: string;
        container?: HTMLElement;
        persistKey?: string;
        closeFolders?: boolean;
    } = {}) {
        const guiOpts: Record<string, unknown> = {};
        if (options.title)     guiOpts['title']        = options.title;
        if (options.container) guiOpts['container']    = options.container;
        if (options.closeFolders) guiOpts['closeFolders'] = true;
        this.gui = new GUI(guiOpts);
        this.folder = new FolderImpl(this.gui, options.persistKey ?? null);
    }

    add<T extends object>(target: T, prop: keyof T & string, ...args: unknown[]): Controller {
        return this.folder.add(target, prop, ...args);
    }
    addNumber<T extends object>(target: T, prop: keyof T & string,
                                min?: number, max?: number, step?: number): Controller {
        return this.folder.addNumber(target, prop, min, max, step);
    }
    addBoolean<T extends object>(target: T, prop: keyof T & string): Controller {
        return this.folder.addBoolean(target, prop);
    }
    addColor<T extends object>(target: T, prop: keyof T & string): Controller {
        return this.folder.addColor(target, prop);
    }
    addButton(name: string, fn: () => void): Controller {
        return this.folder.addButton(name, fn);
    }
    addFolder(name: string): ParamFolder {
        return this.folder.addFolder(name);
    }
    open(): void { this.gui.open(); }
    close(): void { this.gui.close(); }

    destroy(): void { this.gui.destroy(); }
}
