(() => {
  const renderers = new Map();

  function register(renderer) {
    if (!renderer?.id || typeof renderer.drawDiskGrid !== 'function') {
      throw new Error('A tiling renderer must provide an id and drawDiskGrid function.');
    }
    if (renderers.has(renderer.id)) throw new Error(`Duplicate tiling renderer: ${renderer.id}`);
    renderers.set(renderer.id, Object.freeze(renderer));
  }

  function configuration(source) {
    if (source?.tiling) return source.tiling;
    if (source?.id && source?.parameters) return source;
    return {id:'elliptical-square', version:'1', parameters:{
      centerX:source?.center?.x ?? '0', centerY:source?.center?.y ?? '0'
    }};
  }

  function get(source) {
    const config=configuration(source), renderer=renderers.get(config.id);
    if (!renderer) throw new Error(`No browser renderer is installed for tiling ${config.id}.`);
    return {renderer, config, hasReference:
      typeof renderer.drawReferenceGrid === 'function' && typeof renderer.inverse === 'function'};
  }

  window.HTBTilings={register,get,configuration,list:()=>[...renderers.keys()]};
})();
