#!/usr/bin/env node

const YarrboardClient = require('yarrboard-client');
const program = require('commander');
const delay = (millis) => new Promise(resolve => setTimeout(resolve, millis))

program
    .version(YarrboardClient.version)
    .usage('[OPTIONS]...')
    .option('-h, --host <value>', 'Yarrboard hostname', 'yarrboard.local')
    .option('-u, --user <value>', 'Username', 'admin')
    .option('-p, --pass <value>', 'Password', 'admin')
    .option('-c, --channels <value...>', 'Channel IDs', range(1, 8))
    .option('-d, --delay <value>', 'Delay in ms between commands', 25)
    .option('-l, --login', 'Enable login', true)
    .option('--rgb', 'RGB Fade Channel')
    .option('--toggle', 'Toggle Channel')
    .option('--fade', 'Fade Channel')
    .option('--update <value>', 'Poll for updates (ms)')
    .parse(process.argv);

const options = program.opts();

const yb = new YarrboardClient(options.host, options.user, options.pass, options.login);

function main() {
    yb.start();

    setTimeout(yb.printMessageStats.bind(yb), 1);
    twerkIt();
}

function waitUntilOpen() {
    return new Promise(async (resolve) => {
        // Check if yb.isOpen() is already true
        if (yb.isOpen()) {
            resolve();
        } else {
            // Set up a loop to periodically check yb.isOpen()
            const intervalId = setInterval(() => {
                if (yb.isOpen()) {
                    clearInterval(intervalId);
                    resolve();
                }
            }, 100); // You can adjust the interval as needed
        }
    });
}


async function testFadeInterrupt() {
    while (true) {
        yb.fadePWMChannel(2, 1, 1000);
        await delay(500);

        yb.fadePWMChannel(2, 0, 1000);
        await delay(500);
    }
}

async function twerkIt() {
    await waitUntilOpen();

    if (options.rgb) {
        console.log(`RGB Fade on channel ${options.channels} / delay ${options.delay}ms`)
        rgbFade(options.channels, options.delay);
    }
    else if (options.toggle) {
        console.log(`Pin Toggle on channel ${options.channels} / delay ${options.delay}ms`)
        setTimeout(function () { togglePin(options.channels, options.delay) }, 50);
    }
    else if (options.fade) {
        console.log(`Hardware Fade on channel ${options.channels} / delay ${options.delay}ms`)
        setTimeout(function () { fadePinHardware(options.channels, options.delay) }, 50);
    }

    if (options.update)
        yb.startUpdatePoller(options.update);
}

async function togglePin(channels, d = 10) {
    while (true) {
        for (let j = 0; j < channels.length; j++) {
            let channel = channels[j];

            yb.togglePWMChannel(channel, options.host.split(".")[0]);
            await delay(d)
        }
    }
}

async function rgbFade(channels, d = 25) {
    let steps = 25;
    let max_duty = 1;

    while (true) {
        for (let j = 0; j < channels.length; j++) {
            let channel = channels[j];

            for (i = 0; i <= steps; i++) {
                let duty = (i / steps) * max_duty;
                yb.setRGB(channel, duty, 0, 0, false);
                await delay(d)
            }

            for (i = steps; i >= 0; i--) {
                let duty = (i / steps) * max_duty;
                yb.setRGB(channel, duty, 0, 0, false);
                await delay(d)
            }

            for (i = 0; i <= steps; i++) {
                let duty = (i / steps) * max_duty;
                yb.setRGB(channel, 0, duty, 0, false);
                await delay(d)
            }

            for (i = steps; i >= 0; i--) {
                let duty = (i / steps) * max_duty;
                yb.setRGB(channel, 0, duty, 0, false);
                await delay(d)
            }

            for (i = 0; i <= steps; i++) {
                let duty = (i / steps) * max_duty;
                yb.setRGB(channel, 0, 0, duty, false);
                await delay(d)
            }

            for (i = steps; i >= 0; i--) {
                let duty = (i / steps) * max_duty;
                yb.setRGB(channel, 0, 0, duty, false);
                await delay(d)
            }

            yb.setRGB(channel, 0, 0, 0);
            await delay(d);
        }
    }
}

async function fadePinHardware(channels, d = 250, knee = 50) {
    if (!knee)
        knee = d / 2;

    while (true) {
        for (let j = 0; j < channels.length; j++) {
            let channel = channels[j];

            //yb.log(`fading to 1 in ${d}ms`);
            yb.fadePWMChannel(channel, 1, d);
            await delay(d + knee);

            //yb.log(`fading to 0 in ${d}ms`);
            yb.fadePWMChannel(channel, 0, d);
            await delay(d + knee);
        }
    }
}

function range(start, end) {
    var ans = [];
    for (let i = start; i <= end; i++) {
        ans.push(i);
    }
    return ans;
}

// async function testAllFade(d = 1000) {
//     while (true) {
//         for (let i = 0; i < 8; i++)
//             yb.fadePWMChannel(i, 1, d);

//         await delay(d * 2);

//         for (let i = 0; i < 8; i++)
//             yb.fadePWMChannel(i, 0, d);
//         await delay(d * 2);
//     }
// }

// async function speedTest(msg, delay_ms = 10) {
//     while (true) {
//         yb.json(msg);
//         await delay(delay_ms);
//     }
// }

// async function exercisePins() {
//     while (true) {
//         for (i = 0; i < 8; i++) {
//             yb.setPWMChannelDuty(i, Math.random());
//             yb.setPWMChannelState(i, true);

//             await delay(200)
//         }

//         for (i = 0; i < 8; i++) {
//             yb.setPWMChannelState(i, false);
//             await delay(200)
//         }
//     }
// }

// async function fadePinManual(channel = 0, d = 10) {
//     let steps = 25;
//     let max_duty = 1;

//     while (true) {
//         yb.setPWMChannelState(channel, true);
//         await delay(d)

//         for (i = 0; i <= steps; i++) {
//             yb.setPWMChannelDuty(channel, (i / steps) * max_duty)
//             await delay(d)
//         }

//         for (i = steps; i >= 0; i--) {
//             yb.setPWMChannelDuty(channel, (i / steps) * max_duty)
//             await delay(d)
//         }

//         await delay(d)
//     }
// }

main();