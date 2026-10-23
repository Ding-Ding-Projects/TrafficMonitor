// Offline presentation only. Release values remain literal producer data.
const copy = {
  snapshot: ['Bundled release snapshot', '隨附版本資料快照'],
  installerKind: ['x64 · Unsigned installer', 'x64 · 未簽署安裝程式'],
  warning: ['Release information was bundled with this reader and may be out of date. External links do not open in the offline reader.', '版本資料隨閱讀器一同封裝，可能已經過時。離線閱讀器唔會開啟外部連結。'],
  installer: ['Installer link from bundled snapshot', '隨附快照內嘅安裝程式連結'],
  source: ['Documentation source', '文件來源版本'],
  checked: ['Bundled release metadata checked', '隨附版本資料核實時間'],
  unavailable: ['Check time unavailable', '未提供核實時間'],
  published: ['Snapshot release published', '快照版本發佈時間'],
  status: ['This recorded release is not a draft or prerelease. Publication does not establish completion of every planned feature.', '呢個已記錄版本唔係草稿或預覽版。發佈唔代表所有計劃功能已經完成。']
};
export function isOfflineReader(address) {
  try {
    const url = new URL(address);
    return url.protocol === 'traffic-docs:' && url.hostname === 'reader' && !url.port && !url.username && !url.password;
  } catch { return false; }
}
export function applyOfflineReleaseLabels(document, address) {
  if (!isOfflineReader(address) || document.body.dataset.offlineReleaseLabels === 'applied') return false;
  document.body.dataset.offlineReleaseLabels = 'applied';
  const label = key => {
    const node = document.createElement('span');
    [node.dataset.en, node.dataset.yue] = copy[key];
    node.textContent = copy[key][0];
    return node;
  };
  const literal = text => {
    const node = document.createElement('span');
    node.dataset.originalContent = '';
    node.textContent = text;
    return node;
  };
  const checked = value => value ? literal(value) : label('unavailable');
  for (const node of document.querySelectorAll('[data-release-tag]')) {
    const tag = node.textContent;
    node.replaceChildren(label('snapshot'), ': ', literal(tag));
  }
  for (const node of document.querySelectorAll('[data-release-home]')) {
    const original = node.firstChild;
    original.replaceWith(label('snapshot'), ': ', literal(node.dataset.releaseTagValue), ' · ', label('installerKind'), ' · ');
  }
  for (const node of document.querySelectorAll('[data-release-eyebrow]')) node.replaceChildren(label('snapshot'));
  for (const node of document.querySelectorAll('[data-release-provenance]')) {
    node.replaceChildren(label('source'), ': ', literal(node.dataset.source), ' · ', label('checked'), ': ', checked(node.dataset.checkTime));
  }
  for (const node of document.querySelectorAll('[data-release-facts]')) {
    node.replaceChildren(label('published'), ': ', literal(node.dataset.published), '. ', label('checked'), ': ', checked(node.dataset.checkTime), '. ', label('status'));
  }
  for (const button of document.querySelectorAll('[data-snapshot-installer]')) {
    [button.dataset.en, button.dataset.yue] = copy.installer;
    button.textContent = copy.installer[0];
    // The Material button keeps its href and native shadow-anchor semantics.
    button.removeAttribute('aria-label');
  }
  for (const anchor of document.querySelectorAll('[data-release-home],[data-release-facts]')) {
    const notice = document.createElement('p');
    notice.dataset.offlineReleaseWarning = '';
    notice.className = 'muted';
    notice.append(label('warning'));
    anchor.after(notice);
  }
  return true;
}
