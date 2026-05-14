export class Widget {
    name: string;
    count: number;

    constructor(name: string) {
        this.name = name;
        this.count = 0;
    }

    increment(): void {
        this.count += 1;
    }
}

export function makeWidget(name: string): Widget {
    return new Widget(name);
}
