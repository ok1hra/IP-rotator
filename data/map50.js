(function(){
  function mapDatasetError(message){
    try {
      window.dispatchEvent(new CustomEvent('map50-error', { detail: String(message) }));
    } catch (err) {
      console.error(message);
    }
  }
  fetch('/map50.js.gz').then(function(response){
    if(!response.ok){
      throw new Error('Map dataset request failed.');
    }
    return response.arrayBuffer();
  }).then(function(buffer){
    var bytes = new Uint8Array(buffer);
    var isGzip = bytes.length >= 2 && bytes[0] === 0x1f && bytes[1] === 0x8b;
    if(!isGzip){
      return new TextDecoder('utf-8').decode(bytes);
    }
    if (typeof DecompressionStream !== 'function') {
      throw new Error('Browser does not support gzip map loading.');
    }
    var stream = new Response(buffer).body.pipeThrough(new DecompressionStream('gzip'));
    return new Response(stream).text();
  }).then(function(source){
    (0, eval)(source);
    window.dispatchEvent(new Event('map50-ready'));
  }).catch(function(error){
    console.error(error);
    mapDatasetError(error && error.message ? error.message : error);
  });
})();
