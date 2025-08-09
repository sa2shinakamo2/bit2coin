// Parallax effect for aurora via CSS variables
(function(){
  const root = document.documentElement;
  window.addEventListener('mousemove', (e) => {
    const x = (e.clientX / window.innerWidth - 0.5);
    const y = (e.clientY / window.innerHeight - 0.5);
    root.style.setProperty('--mx', (x*1).toFixed(4));
    root.style.setProperty('--my', (y*1).toFixed(4));
  });
})();
