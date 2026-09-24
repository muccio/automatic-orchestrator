// Hollywood Orchestrator UI Controller
document.addEventListener('DOMContentLoaded', () => {
    // Populate 16-step visual sequencer grids
    const grids = document.querySelectorAll('.step-grid');
    grids.forEach(grid => {
        for (let i = 0; i < 16; ++i) {
            const step = document.createElement('div');
            step.className = 'step-box';
            if (i % 4 === 0) step.classList.add('beat-accent');

            // Default ostinato pattern: active on all steps, higher on beat accents
            step.classList.add('active');
            const velBar = document.createElement('div');
            velBar.className = 'step-vel-bar';
            const height = (i % 4 === 0) ? 80 : ((i % 2 === 0) ? 60 : 45);
            velBar.style.height = `${height}%`;
            step.appendChild(velBar);

            step.addEventListener('click', () => {
                step.classList.toggle('active');
                velBar.style.display = step.classList.contains('active') ? 'block' : 'none';
            });

            grid.appendChild(step);
        }
    });

    // Slider value synchronizers
    const setupSliderSync = (sliderId, labelId, suffix = '') => {
        const slider = document.getElementById(sliderId);
        const label = document.getElementById(labelId);
        if (slider && label) {
            slider.addEventListener('input', () => {
                label.textContent = `${slider.value}${suffix}`;
            });
        }
    };

    setupSliderSync('sliderJitter', 'valJitter', '%');
    setupSliderSync('sliderColor', 'valColor', '%');
    setupSliderSync('sliderHuman', 'valHuman', '');
    setupSliderSync('sliderGrace', 'valGrace', 'ms');

    // Drag and Drop simulation hook for browser testing
    const masterDrag = document.getElementById('btnMasterDrag');
    if (masterDrag) {
        masterDrag.addEventListener('dragstart', (e) => {
            e.dataTransfer.setData('text/plain', 'HollywoodOrchestrator_Master.mid');
            console.log('Initiating Master MIDI Drag Drop (16 Channels)');
        });
    }

    // Stem buttons
    document.querySelectorAll('.stem-drag-btn').forEach(btn => {
        btn.setAttribute('draggable', 'true');
        btn.addEventListener('dragstart', (e) => {
            const stem = btn.getAttribute('data-stem');
            e.dataTransfer.setData('text/plain', `HollywoodOrchestrator_${stem}.mid`);
            console.log(`Initiating Stem MIDI Drag Drop: ${stem}`);
        });
    });

    // Simulate chord monitor updates for demo
    const chordBadge = document.getElementById('chordBadge');
    const chordSubtext = document.getElementById('chordSubtext');
    const demoChords = [
        { name: "C min 9", sub: "C - Eb - G - Bb - D (Dorian Mode)" },
        { name: "Ab Maj 7", sub: "Ab - C - Eb - G" },
        { name: "F min 7", sub: "F - Ab - C - Eb" },
        { name: "G 7 alt / B", sub: "B - F - Ab - Db (Inverted Altered Dominant)" }
    ];

    let chordIdx = 0;
    setInterval(() => {
        if (window.__ORCHESTRATOR_NATIVE_IPC__) {
            // Handled via native JUCE IPC
            return;
        }
        const c = demoChords[chordIdx % demoChords.length];
        if (chordBadge) chordBadge.textContent = c.name;
        if (chordSubtext) chordSubtext.textContent = c.sub;
        chordIdx++;
    }, 4000);
});
