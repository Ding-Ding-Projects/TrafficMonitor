import { execFileSync } from 'node:child_process';
import { createHash } from 'node:crypto';
import { readFileSync, writeFileSync } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const git = (...args) => execFileSync('git', args, { cwd: root, maxBuffer: 32 * 1024 * 1024 });
const manifestPath = resolve(root, 'docs/site-content/manifest.json');
const previous = JSON.parse(readFileSync(manifestPath, 'utf8').replace(/^\uFEFF/, ''));
const sourceCommit = git('rev-parse', 'HEAD').toString().trim();
if (!/^[a-f0-9]{40}$/.test(sourceCommit)) throw new Error('Invalid source revision');
const files = git('ls-tree', '-rz', '--name-only', sourceCommit).toString().split('\0').filter(Boolean);
const existing = new Map(previous.articles.map(article => [article.sourcePath, article]));
const publicationPath = 'docs/wiki/pending/publication.json';
const publication = files.includes(publicationPath) ? JSON.parse(git('show', `${sourceCommit}:${publicationPath}`).toString()) : null;
const wikiUrls = new Map();
if (publication?.state === 'published-verified') {
  if (!/^https:\/\/github\.com\/[^/]+\/[^/]+\.wiki\.git$/.test(publication.destination) || !/^[a-f0-9]{40}$/.test(publication.publicationProof?.commit ?? '')) throw new Error('Invalid published wiki provenance');
  for (const page of publication.pages) {
    if (!/^[\p{L}\p{N}_ -]+\.md$/u.test(page)) throw new Error('Invalid wiki page name');
    wikiUrls.set(`docs/wiki/pending/${page}`, `${publication.destination.replace(/\.wiki\.git$/, '/wiki')}/${encodeURIComponent(page.slice(0, -3))}`);
  }
}
const repositoryPaths = files.filter(path => /\.md$/i.test(path) && !path.startsWith('docs/wiki/upstream/'));
const categoryFor = path => {
  if (/^design\//.test(path)) return 'design-reference';
  if (/^docs\/build\//.test(path)) return 'building';
  if (/^docs\/features\/required-features\//.test(path)) return 'required-features';
  if (/^docs\/features\/installation\//.test(path)) return 'getting-started';
  if (/^evidence\//.test(path)) return 'evidence';
  return 'contributor-records';
};
const articles = repositoryPaths.map(path => {
  const bytes = git('show', `${sourceCommit}:${path}`);
  const text = bytes.toString('utf8').replace(/^\uFEFF/, '');
  const original = existing.get(path);
  const id = original?.id ?? `source-${path.replace(/\.md$/i, '').toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '')}`;
  if (id === 'source-') throw new Error(`A stable article ID must be assigned for ${path}`);
  return {
    ...original,
    id,
    title: /^#\s+(.+)$/m.exec(text)?.[1]?.trim() ?? original?.title ?? path,
    category: original?.category ?? categoryFor(path),
    language: original?.language ?? 'en',
    sourcePath: path,
    originalSourcePath: path,
    route: original?.route ?? `/articles/${id}/`,
    sourceUrl: `https://github.com/Ding-Ding-Projects/TrafficMonitor/blob/${sourceCommit}/${path.split('/').map(encodeURIComponent).join('/')}`,
    sourceSha256: createHash('sha256').update(bytes).digest('hex'),
    provenanceKind: 'repository-markdown',
    contentStatus: 'complete-source-article',
    historicalPresentation: original?.historicalPresentation ?? false,
    ...(wikiUrls.has(path) ? { wikiSourceUrl: wikiUrls.get(path) } : {}),
  };
});
articles.push(...previous.articles.filter(article => article.provenanceKind !== 'repository-markdown'));
for (const key of ['id', 'route', 'sourcePath']) {
  if (new Set(articles.map(article => article[key])).size !== articles.length) throw new Error(`Duplicate article ${key}`);
}
const next = {
  ...previous,
  sourceCommit,
  articles,
  counts: {
    ...previous.counts,
    repositoryMarkdownArticles: repositoryPaths.length,
    wikiMarkdownArticles: articles.length - repositoryPaths.length,
    articles: articles.length,
  },
  categories: [...new Set(articles.map(article => article.category))].sort(),
};
writeFileSync(manifestPath, `${JSON.stringify(next, null, 2)}\n`, 'utf8');
console.log(`Content inventory refreshed: ${repositoryPaths.length} repository articles, ${articles.length - repositoryPaths.length} wiki articles, source ${sourceCommit}.`);
