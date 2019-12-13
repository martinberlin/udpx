brotliwasm.load().then(({ compress }) => {
    Object.assign(window, { compress });
})

const buffer_size = 4096;
const quality_default = 11;
const lg_window_size = 21;

