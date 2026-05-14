// Mirrors ParticleFrame::radii defect: radii is declared but never read.
export class ParticleFrame {
    positions: number[] = [];
    velocities: number[] = [];
    radii: number[] = [];
    ids: number[] = [];

    constructor(n: number) {
        this.positions = new Array(n * 3);
        this.velocities = new Array(n * 3);
        this.radii = new Array(n);
        this.ids = new Array(n);
    }
}
