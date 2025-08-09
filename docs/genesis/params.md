# Genesis Parameters

The parameters below are sourced from `genesis/genesis_params.yaml` and rendered client‑side for transparency and ease of reading.

<div id="genesis-yaml" class="genesis-yaml"></div>

<script>
(function(){
  const target = document.getElementById('genesis-yaml');
  if(!target) return;
  fetch('/genesis/genesis_params.yaml', { cache: 'no-store' })
    .then(r => {
      if(!r.ok) throw new Error('Failed to fetch genesis_params.yaml');
      return r.text();
    })
    .then(text => {
      const pre = document.createElement('pre');
      const code = document.createElement('code');
      code.className = 'language-yaml';
      code.textContent = text;
      pre.appendChild(code);
      target.appendChild(pre);
      // trigger highlight if available (mkdocs-material provides highlight.js)
      if (window.hljs) { window.hljs.highlightElement(code); }
    })
    .catch(err => {
      target.innerHTML = '<div class="admonition danger"><p class="admonition-title">Error</p><p>'+ (err && err.message ? err.message : 'Unable to load YAML') +'</p></div>';
    });
})();
</script>

<style>
.genesis-yaml pre {
  border-radius: 10px;
}
</style>
