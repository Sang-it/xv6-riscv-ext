import { test, expect } from '@playwright/test';

test('console, custom commands, disk, and reload', async ({ page }) => {
  const errors = [];
  page.on('pageerror', error => errors.push(error.message));
  page.on('response', response => { if (response.status() >= 400) errors.push(`${response.status()} ${response.url()}`); });
  await page.goto('/');
  const console = page.locator('#terminal');
  await expect(console).toHaveAttribute('aria-busy', 'false', { timeout: 90_000 });
  const screen = page.locator('.xterm-accessibility-tree');
  await page.keyboard.type('ps');
  await page.keyboard.press('Enter');
  await expect(screen).toContainText('PPID');
  await expect(screen).toContainText('init');
  await page.keyboard.type('c4 hello.c');
  await page.keyboard.press('Enter');
  await expect(screen).toContainText('5! = 120');
  await expect(screen).toContainText('string: AAAA');

  await page.keyboard.type('echo persisted-in-session > proof');
  await page.keyboard.press('Enter');
  await page.keyboard.type('cat proof');
  await page.keyboard.press('Enter');
  // Type-ahead can echo the second command before the shell prints its prompt.
  // Require a standalone readback line, allowing that prompt before the value.
  await expect(screen.getByRole('listitem').filter({ hasText: /^(?:\$ )?persisted-in-session\s*$/ })).toHaveCount(1);
  await page.screenshot({ path: 'test-results/desktop.png', fullPage: true });

  await page.reload();
  await expect(console).toHaveAttribute('aria-busy', 'false', { timeout: 90_000 });
  await page.keyboard.type('cat proof');
  await page.keyboard.press('Enter');
  await expect(screen).toContainText('cannot open proof');
  expect(errors).toEqual([]);
});

test('mobile layout and missing-asset recovery', async ({ page }) => {
  await page.setViewportSize({ width: 390, height: 844 });
  await page.route('**/assets/kernel.bin', route => route.fulfill({ status: 404, body: 'missing' }));
  await page.goto('/');
  await expect(page.locator('.xterm-accessibility-tree')).toContainText('Could not load kernel.bin');
  await page.unroute('**/assets/kernel.bin');
  await page.reload();
  await expect(page.locator('#terminal')).toHaveAttribute('aria-busy', 'false', { timeout: 90_000 });
  await page.keyboard.type('pwd');
  await page.keyboard.press('Enter');
  await expect(page.locator('.xterm-accessibility-tree')).toContainText('pwd');
  expect(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth)).toBe(true);
  await expect(page.getByRole('button')).toHaveCount(0);
  const bounds = await page.locator('#terminal').boundingBox();
  expect(bounds.width).toBe(390);
  expect(bounds.height).toBe(844);
  await page.screenshot({ path: 'test-results/mobile.png', fullPage: true });
});
