import GUI from 'lil-gui';
import type { Controller } from 'lil-gui';
import { log } from './log.js';

/**
 * Controller returned by ParamPanel methods. Adds a `label` setter that
 * delegates to lil-gui's `name(text)` — lets sim code write
 * `panel.addNumber(rt, 'x').label = 'X axis'` ergonomically.
 */
export type LabeledController = Controller & { label: string };

export interface DropdownOption {
    value: string;
    label: string;
}

export interface DropdownSpec {
    /** Read the current value (called once when the controller is created). */
    getValue: () => string;
    /** Write a new value (called when the user picks an option). */
    setValue: (v: string) => void;
    /** Display label for the row. */
    label: string;
    /** Choices. `value` is what's passed to `setValue`; `label` is shown. */
    options: DropdownOption[];
}

export interface ParamFolder {
    add<T extends object>(target: T, prop: keyof T & string, ...args: unknown[]): LabeledController;
    addFolder(name: string): ParamFolder;
    addBoolean<T extends object>(target: T, prop: keyof T & string): LabeledController;
    addNumber<T extends object>(target: T, prop: keyof T & string,
                                min?: number, max?: number, step?: number): LabeledController;
    addColor<T extends object>(target: T, prop: keyof T & string): LabeledController;
    addDropdown(spec: DropdownSpec): LabeledController;
    addButton(name: string, fn: () => void): LabeledController;
    /**
     * Refresh every controller's displayed value to match its bound source.
     * Call after externally mutating bound state (preset selection, capture
     * load) so lil-gui sliders / dropdowns don't show stale values. Recurses
     * into sub-folders.
     */
    refreshDisplays(): void;
    /** Destroy all controllers and sub-folders. */
    clear(): void;
    open(): void;
    close(): void;
}

function attachLabelSetter(c: Controller): LabeledController {
    // Attach idempotently — re-defining is safe but unnecessary.
    if (Object.prototype.hasOwnProperty.call(c, 'label')) {
        return c as LabeledController;
    }
    Object.defineProperty(c, 'label', {
        configurable: true,
        enumerable: false,
        get(): string {
            return (this as Controller & { _name?: string })._name ?? '';
        },
        set(v: string) {
            (this as Controller).name(v);
        },
    });
    return c as LabeledController;
}

class FolderImpl implements ParamFolder {
    constructor(private gui: GUI, private storageKey: string | null) {}

    add<T extends object>(target: T, prop: keyof T & string, ...args: unknown[]): LabeledController {
        const c = (this.gui.add as (...a: unknown[]) => Controller)(target, prop, ...args);
        if (this.storageKey) this.attachPersistence(c, target, prop);
        return attachLabelSetter(c);
    }

    addNumber<T extends object>(target: T, prop: keyof T & string,
                                min?: number, max?: number, step?: number): LabeledController {
        const c = this.gui.add(target, prop, min as number, max as number, step as number);
        if (this.storageKey) this.attachPersistence(c, target, prop);
        return attachLabelSetter(c);
    }

    addBoolean<T extends object>(target: T, prop: keyof T & string): LabeledController {
        const c = this.gui.add(target, prop);
        if (this.storageKey) this.attachPersistence(c, target, prop);
        return attachLabelSetter(c);
    }

    addColor<T extends object>(target: T, prop: keyof T & string): LabeledController {
        const c = this.gui.addColor(target, prop);
        if (this.storageKey) this.attachPersistence(c, target, prop);
        return attachLabelSetter(c);
    }

    addDropdown(spec: DropdownSpec): LabeledController {
        // Build a {label: value} map for lil-gui's options object form.
        const optionMap: Record<string, string> = {};
        for (const o of spec.options) {
            optionMap[o.label] = o.value;
        }
        // Proxy object with a getter/setter delegating to spec.
        const proxy = {} as Record<string, string>;
        Object.defineProperty(proxy, 'value', {
            enumerable: true,
            configurable: true,
            get(): string { return spec.getValue(); },
            set(v: string) { spec.setValue(v); },
        });
        const c = this.gui.add(proxy, 'value', optionMap).name(spec.label);
        return attachLabelSetter(c);
    }

    addButton(name: string, fn: () => void): LabeledController {
        const obj = { [name]: fn };
        return attachLabelSetter(this.gui.add(obj, name));
    }

    addFolder(name: string): ParamFolder {
        const folder = this.gui.addFolder(name);
        return new FolderImpl(folder, this.storageKey);
    }

    refreshDisplays(): void {
        // lil-gui exposes controllersRecursive() on both root GUI and
        // sub-folder GUI; it returns every Controller at all depths under
        // this scope. The `as unknown as` cast matches the typings-gap
        // pattern used by clear() below — lil-gui 0.20 ships a d.ts that
        // doesn't expose this method or the `children` array.
        const controllers = (this.gui as unknown as {
            controllersRecursive: () => Array<{ updateDisplay: () => void }>;
        }).controllersRecursive();
        let failed = 0;
        for (const c of controllers) {
            try {
                c.updateDisplay();
            } catch (err) {
                // Library code: log loud rather than swallow silently. A real
                // bug here (controller in invalid state, lil-gui shape change)
                // should surface immediately. Log only the first failure per
                // call to avoid console flood when a single broken controller
                // would otherwise spam.
                if (failed === 0) {
                    log.warn('ParamPanel.refreshDisplays: controller updateDisplay() threw', err);
                }
                failed++;
            }
        }
        if (failed > 1) {
            log.warn(`ParamPanel.refreshDisplays: ${failed} controller(s) failed; only the first error was logged`);
        }
    }

    clear(): void {
        // Snapshot children — destroy() mutates the array.
        const kids = (this.gui as unknown as { children: Array<{ destroy: () => void }> }).children.slice();
        for (const c of kids) {
            try { c.destroy(); } catch { /* ignore */ }
        }
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

    add<T extends object>(target: T, prop: keyof T & string, ...args: unknown[]): LabeledController {
        return this.folder.add(target, prop, ...args);
    }
    addNumber<T extends object>(target: T, prop: keyof T & string,
                                min?: number, max?: number, step?: number): LabeledController {
        return this.folder.addNumber(target, prop, min, max, step);
    }
    addBoolean<T extends object>(target: T, prop: keyof T & string): LabeledController {
        return this.folder.addBoolean(target, prop);
    }
    addColor<T extends object>(target: T, prop: keyof T & string): LabeledController {
        return this.folder.addColor(target, prop);
    }
    addDropdown(spec: DropdownSpec): LabeledController {
        return this.folder.addDropdown(spec);
    }
    addButton(name: string, fn: () => void): LabeledController {
        return this.folder.addButton(name, fn);
    }
    addFolder(name: string): ParamFolder {
        return this.folder.addFolder(name);
    }
    refreshDisplays(): void {
        this.folder.refreshDisplays();
    }

    clear(): void {
        this.folder.clear();
    }
    open(): void { this.gui.open(); }
    close(): void { this.gui.close(); }

    destroy(): void { this.gui.destroy(); }
}
