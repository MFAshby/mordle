/**
 * Share results
 * Just like the OG wordle, copy the grid squares to the clipboard with a message
 */ 
function share_results() {
    const chrs = Array.from(document
        .getElementsByClassName('letter'))
        .map(elem => elem.classList)
        .map(classList => Array.from(classList).filter(klass => klass !== 'letter')[0])
        .map(klass => {
            switch (klass) {
                case 'correct': return '🟩';
                case 'present_wrong_pos': return '🟨';
                case 'incorrect': return '⬜';
            }
        });
    const lines = array_chunk(chrs, 5).map(line => line.join(''));
    const date = document.getElementById('date').textContent;
    const score_desc = "wordle " + date + " score " + lines.length + " / 6 \n" + lines.join('\n');
    console.log(score_desc);
    navigator.clipboard.writeText(score_desc);
}

function array_chunk(arr, chunkSize) {
    if (chunkSize <= 0) {
        throw "Invalid chunk size";
    }
    const len = arr.length;
    var result = [];
    for (var i=0; i<len; i+=chunkSize) {
        result.push(arr.slice(i, i+chunkSize));
    }
    return result;
}