void
grid(Monitor *m)
{
    unsigned int i, n, cx, cy, cw, ch, aw, ah, cols, rows;
    Client *c;

    for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next))
        n++;

    /* grid dimensions */
    for (rows = 0; rows <= n / 2; rows++)
        if (rows * rows >= n)
            break;
    cols = (rows && (rows - 1) * rows >= n) ? rows - 1 : rows;

    /* window geoms (cell height/width) */
    ch = m->wh / (rows ? rows : 1);
    cw = m->ww / (cols ? cols : 1);
    for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next))
    {
        cx = m->wx + m->gappx + (i / rows) * cw;
        cy = m->wy + m->gappx + (i % rows) * ch;
        /* adjust height/width of last row/column's windows */
        ah = ((i + 1) % rows == 0) ? m->wh - ch * rows - m->gappx : 0;
        aw = (i >= rows * (cols - 1)) ? m->ww - cw * cols - m->gappx : 0;
        resize(c, cx, cy, cw - 2 * c->bw + aw - m->gappx , ch - 2 * c->bw + ah - m->gappx , False);
        i++;
    }
}
