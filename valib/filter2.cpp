#include "filter2.h"

class FilterThunk : public Filter
{
protected:
  Filter2 *f;
  Chunk2 in_chunk;
  Chunk  out_chunk;

  Speakers spk;
  Speakers out_spk;

  bool flushing;
  bool new_stream;
  bool make_output();

public:
  FilterThunk(Filter2 *f);

  virtual void reset();
  virtual bool is_ofdd() const;

  virtual bool query_input(Speakers spk) const;
  virtual bool set_input(Speakers spk);
  virtual Speakers get_input() const;
  virtual bool process(const Chunk *chunk);

  virtual Speakers get_output() const;
  virtual bool is_empty() const;
  virtual bool get_chunk(Chunk *chunk);
};

FilterThunk::FilterThunk(Filter2 *f_):
f(f_), flushing(false), new_stream(false)
{}

void
FilterThunk::reset()
{
  f->reset();
  out_spk = f->get_output();
  in_chunk.set_empty();
  out_chunk.set_dummy();
  flushing = false;
  new_stream = false;
}

bool
FilterThunk::is_ofdd() const
{
  return f->is_ofdd();
}

bool
FilterThunk::query_input(Speakers new_spk) const
{
  return f->can_open(new_spk);
}

bool
FilterThunk::set_input(Speakers new_spk)
{
  reset();
  if (!f->open(new_spk))
  {
    spk = spk_unknown;
    out_spk = spk_unknown;
    return false;
  }

  f->reset();
  spk = new_spk;
  out_spk = f->get_output();
  return true;
}

Speakers
FilterThunk::get_input() const
{
  if (!f->is_open())
    return spk_unknown;
  return spk;
}

bool
FilterThunk::process(const Chunk *chunk)
{
  assert(in_chunk.is_empty());
  assert(out_chunk.is_dummy());

  // ignore dummy chunks
  if (chunk->is_dummy())
    return true;

  // remember data
  in_chunk.set(chunk->rawdata, chunk->samples, chunk->size,
    chunk->sync, chunk->time);

  // format change
  if (spk != chunk->spk)
  {
    flushing = false;
    new_stream = false;
    if (!f->open(chunk->spk))
    {
      reset();
      spk = spk_unknown;
      out_spk = spk_unknown;
      return false;
    }
    f->reset();
    spk = chunk->spk;
    out_spk = f->get_output();
  }

  // flushing
  if (chunk->eos)
    flushing = true;

  out_chunk.set_dummy();
  while (out_chunk.is_dummy() && (flushing || !in_chunk.is_empty()))
    if (!make_output())
      return false;

  return true;
}

Speakers
FilterThunk::get_output() const
{
  return out_spk;
}

bool
FilterThunk::is_empty() const
{
  return !flushing && !new_stream && in_chunk.is_empty() && out_chunk.is_dummy();
}

bool
FilterThunk::make_output()
{
  Chunk2 out_chunk2;

  // normal processing
  if (!in_chunk.is_empty())
  {
    if (!f->process(in_chunk, out_chunk2))
      return false;

    out_chunk.set(f->get_output(),
      out_chunk2.rawdata, out_chunk2.samples, out_chunk2.size,
      out_chunk2.sync, out_chunk2.time);

    new_stream = f->new_stream();
    if (!new_stream)
      out_spk = f->get_output();
    return true;
  }

  // flushing
  if (flushing)
  {
    if (f->need_flushing())
    {
      f->flush(out_chunk2);
      out_chunk.set(f->get_output(),
        out_chunk2.rawdata, out_chunk2.samples, out_chunk2.size,
        out_chunk2.sync, out_chunk2.time);
      new_stream = f->new_stream();
      if (!new_stream)
        out_spk = f->get_output();
    }
    else
    {
      out_chunk.set_empty(out_spk);
      out_chunk.set_eos(true);
      flushing = false;
      new_stream = false;
    }
    return true;
  }

  // dummy
  out_chunk.set_dummy();
  return true;
}

bool
FilterThunk::get_chunk(Chunk *chunk)
{
  if (new_stream)
  {
    chunk->set_empty(out_spk);
    chunk->set_eos(true);
    out_spk = f->get_output();
    new_stream = false;
    return true;
  }

  if (!out_chunk.is_dummy())
  {
    *chunk = out_chunk;
    out_chunk.set_dummy();
    return true;
  }

  while (out_chunk.is_dummy() && (flushing || !in_chunk.is_empty()))
    if (!make_output())
      return false;

  if (new_stream)
  {
    chunk->set_empty(out_spk);
    chunk->set_eos(true);
    out_spk = f->get_output();
    new_stream = false;
    return true;
  }

  *chunk = out_chunk;
  out_chunk.set_dummy();
  return true;
}

///////////////////////////////////////////////////////////////////////////////

Filter2::Filter2()
{
  thunk = new FilterThunk(this);
}

Filter2::~Filter2()
{
  safe_delete(thunk);
}

Filter2::operator Filter *()
{ return thunk; }

Filter *Filter2::operator ->()
{ return thunk; }

Filter2::operator const Filter *() const
{ return thunk; }

const Filter *Filter2::operator ->() const
{ return thunk; }
