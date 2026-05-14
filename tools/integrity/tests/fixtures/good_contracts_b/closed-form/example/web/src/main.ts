import { Widget, makeWidget } from '../../../../common/common-web/src/index.js';

export function useThem(): number {
    const w: Widget = makeWidget('hello');
    w.increment();
    return w.count + w.name.length;
}
